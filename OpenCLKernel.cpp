#ifdef _WIN64
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include <map>
#include <stdio.h>

#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
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

static HMODULE GetThisDllHandle()
{
	MEMORY_BASIC_INFORMATION info;
	size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
	return len ? (HMODULE)info.AllocationBase : NULL;
}

void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Params, const float* p_Input, float* p_Output)
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
    if (kernelMap.find(cmdQ) == kernelMap.end())
	{
		cl_context clContext = NULL;
		error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_CONTEXT, sizeof(cl_context), &clContext, NULL);
        CheckError(error, "Unable to get the context");

		cl_program program = clCreateProgramWithSource(clContext, 1, (const char**)&KernelSource, NULL, &error);
        CheckError(error, "Unable to create program");

        error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        CheckError(error, "Unable to build program");

        kernel = clCreateKernel(program, "GoproVRKernel", &error);
        CheckError(error, "Unable to create kernel");

        kernelMap[cmdQ] = kernel;

    }
    else
    {
        kernel = kernelMap[cmdQ];
    }


    locker.Unlock();
    int count = 0;
	/*
	kernel.setArg(count++, sizeof(int), &p_Width);
	kernel.setArg(count++, sizeof(int), &p_Height);
	kernel.setArg(count++, sizeof(float), &p_Params[0]);
	kernel.setArg(count++, sizeof(float), &p_Params[1]);
	kernel.setArg(count++, sizeof(float), &p_Params[2]);
	kernel.setArg(count++, sizeof(float), &p_Params[3]);
	kernel.setArg(count++, sizeof(cl_mem), &p_Input);
	kernel.setArg(count++, sizeof(cl_mem), &p_Output);*/

    error  = clSetKernelArg(kernel, count++, sizeof(int), &p_Width);
    error |= clSetKernelArg(kernel, count++, sizeof(int), &p_Height); 
	error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Params[0]);
	error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Params[1]);
	error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Params[2]);
	error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Params[3]);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Input);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Output);
    CheckError(error, "Unable to set kernel arguments");

    size_t localWorkSize[2], globalWorkSize[2];
    clGetKernelWorkGroupInfo(kernel, deviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), localWorkSize, NULL);
    localWorkSize[1] = 1;
    globalWorkSize[0] = ((p_Width + localWorkSize[0] - 1) / localWorkSize[0]) * localWorkSize[0];
    globalWorkSize[1] = p_Height;

    clEnqueueNDRangeKernel(cmdQ, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);

	//cl::CommandQueue queue(cmdQ);

	/*queue.enqueueNDRangeKernel(
		kernel,
		cl::NullRange,
		cl::NDRange(p_Width, p_Height),
		cl::NullRange
		);*/
}
