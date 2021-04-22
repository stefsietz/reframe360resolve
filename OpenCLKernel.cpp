#include <map>
#include <stdio.h>
#include <string>

#ifdef _WIN64
#include <Windows.h>
#else
#include <pthread.h>
#endif

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "OpenCLKernel.h"

#define MAX_SOURCE_SIZE (0x100000)

void CheckError(cl_int p_Error, const char* p_Msg)
{
    if (p_Error != CL_SUCCESS)
    {
        fprintf(stderr, "%s [%d]\n", p_Msg, p_Error);
    }
}

class Locker
{
public:
    Locker()
    {
#ifdef _WIN64
        InitializeCriticalSection(&mutex);
#else
        pthread_mutex_init(&mutex, NULL);
#endif
    }

    ~Locker()
    {
#ifdef _WIN64
        DeleteCriticalSection(&mutex);
#else
        pthread_mutex_destroy(&mutex);
#endif
    }

    void Lock()
    {
#ifdef _WIN64
        EnterCriticalSection(&mutex);
#else
        pthread_mutex_lock(&mutex);
#endif
    }

    void Unlock()
    {
#ifdef _WIN64
        LeaveCriticalSection(&mutex);
#else
        pthread_mutex_unlock(&mutex);
#endif
    }

private:
#ifdef _WIN64
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
};

#ifdef _WIN64
static HMODULE GetThisDllHandle()
{
	MEMORY_BASIC_INFORMATION info;
	size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
	return len ? (HMODULE)info.AllocationBase : NULL;
}
#endif

void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, const float* p_Input, float* p_Output, float* p_RotMat, int p_Samples, bool p_Bilinear)
{
    cl_int error;

    cl_command_queue cmdQ = static_cast<cl_command_queue>(p_CmdQ);

    // store device id and kernel per command queue (required for multi-GPU systems)
    static std::map<cl_command_queue, cl_device_id> deviceIdMap;
	static std::map<cl_command_queue, cl_kernel> kernelMap;

    static Locker locker; // simple lock to control access to the above maps from multiple threads

    locker.Lock();

    // find the device id corresponding to the command queue
    cl_device_id deviceId = NULL;
    if (deviceIdMap.find(cmdQ) == deviceIdMap.end())
    {
        error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_DEVICE, sizeof(cl_device_id), &deviceId, NULL);
        CheckError(error, "Unable to get the device");

        deviceIdMap[cmdQ] = deviceId;
    }
    else
    {
        deviceId = deviceIdMap[cmdQ];
    }


    // find the program kernel corresponding to the command queue
    cl_kernel kernel;
	cl_context clContext = NULL;
	error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_CONTEXT, sizeof(cl_context), &clContext, NULL);
	CheckError(error, "Unable to get the context");
    if (kernelMap.find(cmdQ) == kernelMap.end())
	{
		cl_program program = clCreateProgramWithSource(clContext, 1, (const char**)&KernelSource, NULL, &error);
        CheckError(error, "Unable to create program");

        error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
		CheckError(error, "Unable to build program");

		if (error == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, deviceId, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = (char *)malloc(log_size);

			// Get the log
			clGetProgramBuildInfo(program, deviceId, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			std::string log_str(log);

			// Print the log
			printf("%s\n", log);
		}

        kernel = clCreateKernel(program, "Reframe360Kernel", &error);
        CheckError(error, "Unable to create kernel");

        kernelMap[cmdQ] = kernel;

    }
    else
    {
        kernel = kernelMap[cmdQ];
    }

	int bilinear(p_Bilinear ? 1 : 0);

    locker.Unlock();
    int count = 0;

    error  = clSetKernelArg(kernel, count++, sizeof(int), &p_Width);
	error |= clSetKernelArg(kernel, count++, sizeof(int), &p_Height);

	cl_mem fov_buf = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*p_Samples, p_Fov, &error);
	cl_mem tinyplanet_buf = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*p_Samples, p_Tinyplanet, &error);
	cl_mem rectilinear_buf = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*p_Samples, p_Rectilinear, &error);
	error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &fov_buf);
	error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &tinyplanet_buf);
	error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &rectilinear_buf);

    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Input);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Output);

	cl_mem rotmat_buf = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*9*p_Samples, p_RotMat, &error);
	error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &rotmat_buf);
	error |= clSetKernelArg(kernel, count++, sizeof(int), &p_Samples);
	error |= clSetKernelArg(kernel, count++, sizeof(int), &bilinear);

    CheckError(error, "Unable to set kernel arguments");

    size_t localWorkSize[2], globalWorkSize[2];
    clGetKernelWorkGroupInfo(kernel, deviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), localWorkSize, NULL);
    localWorkSize[1] = 1;
    globalWorkSize[0] = ((p_Width + localWorkSize[0] - 1) / localWorkSize[0]) * localWorkSize[0];
    globalWorkSize[1] = p_Height;

	cl_event clEvent;
    clEnqueueNDRangeKernel(cmdQ, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, &clEvent);

	clWaitForEvents(1, &clEvent);
	clReleaseMemObject(fov_buf);
	clReleaseMemObject(tinyplanet_buf);
	clReleaseMemObject(rectilinear_buf);
	clReleaseMemObject(rotmat_buf);
}