#include "helper_math.h"

#define PI 3.1415926535897932384626433832795

__device__ float3 matMul(const float3 r012, const float3 r345, const float3 r678, float3 v){
	float3 outvec = { 0, 0, 0 };
	outvec.x = r012.x * v.x + r012.y * v.y + r012.z * v.z;
	outvec.y = r345.x * v.x + r345.y * v.y + r345.z * v.z;
	outvec.z = r678.x * v.x + r678.y * v.y + r678.z * v.z;
	return outvec;
}

__device__ float2 repairUv(float2 uv){
	float2 outuv = {0, 0};

	if(uv.x<0) {
		outuv.x = 1.0 + uv.x;
		}else if(uv.x > 1.0){
			outuv.x = uv.x -1.0;
		} else {
			outuv.x = uv.x;
		}

		if(uv.y<0) {
			outuv.y = 1.0 + uv.y;
		} else if(uv.y > 1.0){
			outuv.y = uv.y -1.0;
		} else {
			outuv.y = uv.y;
		}

	return outuv;
}

__device__ float2 polarCoord(float3 dir) {	
	float3 ndir = normalize(dir);
	float longi = -atan2(ndir.z, ndir.x);
	
	float lat = acos(-ndir.y);
	
	float2 uv;
	uv.x = longi;
	uv.y = lat;
	
	float2 pitwo = {PI, PI};
	uv /= pitwo;
	uv.x /= 2.0;
	float2 ones = {1.0, 1.0};
	uv = fmodf(uv, ones);
	return uv;
}


__device__ float3 fisheyeDir(float3 dir, const float3 r012, const float3 r345, const float3 r678) {
	
	dir.x = dir.x / dir.z;
	dir.y = dir.y / dir.z;
	dir.z = dir.z / dir.z;
	
	float2 uv;
	uv.x = dir.x;
	uv.y = dir.y;
	float r = sqrtf(uv.x*uv.x + uv.y*uv.y);
	
	float phi = atan2f(uv.y, uv.x);
	
	float theta = r;
	
	float3 fedir = { 0, 0, 0 };
	fedir.x = sin(theta) * cos(phi);
	fedir.y = sin(theta) * sin(phi);
	fedir.z = cos(theta);

	fedir = matMul(r012, r345, r678, fedir);
	
	return fedir;
}

__device__ float4 linInterpCol(float2 uv, const float* input, int width, int height){
	float4 outCol = {0,0,0,0};
	float i = floor(uv.x);
	float j = floor(uv.y);
	float a = uv.x-i;
	float b = uv.y-j;
	int x = (int)i;
	int y = (int)j;
	const int indexX1Y1 = ((y * width) + x) * 4;
	const int indexX2Y1 = ((y * width) + x+1) * 4;
	const int indexX1Y2 = (((y+1) * width) + x) * 4;
	const int indexX2Y2 = (((y+1) * width) + x+1) * 4;
	const int maxIndex = (width * height -1) * 4;
	
	if(indexX2Y2 < maxIndex-height - 100){
		outCol.x = (1.0 - a)*(1.0 - b)*input[indexX1Y1] + a*(1.0 - b)*input[indexX2Y1] + (1.0 - a)*b*input[indexX1Y2] + a*b*input[indexX2Y2];
		outCol.y = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 1] + a*(1.0 - b)*input[indexX2Y1 + 1] + (1.0 - a)*b*input[indexX1Y2 + 1] + a*b*input[indexX2Y2 + 1];
		outCol.z = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 2] + a*(1.0 - b)*input[indexX2Y1 + 2] + (1.0 - a)*b*input[indexX1Y2 + 2] + a*b*input[indexX2Y2 + 2];
		outCol.w = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 3] + a*(1.0 - b)*input[indexX2Y1 + 3] + (1.0 - a)*b*input[indexX1Y2 + 3] + a*b*input[indexX2Y2 + 3];
	} else {
		outCol.x = input[indexX1Y1];
		outCol.y = input[indexX1Y1+ 1];
		outCol.z = input[indexX1Y1+ 2];
		outCol.w = input[indexX1Y1+ 3];
	}
	return outCol;
}

__global__ void GainAdjustKernel(int p_Width, int p_Height, float* p_Fov, float* p_Fisheye,
								const float* p_Input, float* p_Output, const float* r, int samples)
{
   const int x = blockIdx.x * blockDim.x + threadIdx.x;
   const int y = blockIdx.y * blockDim.y + threadIdx.y;

   if ((x < p_Width) && (y < p_Height))
   {
		const int index = ((y * p_Width) + x) * 4;

		float4 accum_col = {0, 0, 0, 0};

		for(int i=0; i<samples; i++){
			float fov = p_Fov[i];

		   float2 uv = { (float)x / p_Width, (float)y / p_Height };
		   float aspect = (float)p_Width / (float)p_Height;

		   float3 dir = { 0, 0, 0 };
		   dir.x = (uv.x - 0.5)*2.0;
		   dir.y = (uv.y - 0.5)*2.0;
		   dir.y /= aspect;
		   dir.z = fov;

		   const float3 r012 = {r[i*9+0], r[i*9+1], r[i*9+2]};
		   const float3 r345 = {r[i*9+3], r[i*9+4], r[i*9+5]};
		   const float3 r678 = {r[i*9+6], r[i*9+7], r[i*9+8]};

		   float3 rectdir = matMul(r012, r345, r678, dir);

		   rectdir = normalize(rectdir);

		   dir = lerp(rectdir, fisheyeDir(dir, r012, r345, r678), p_Fisheye[i]);

		   float2 iuv = polarCoord(dir);
		   iuv = repairUv(iuv);

		   int x_new = iuv.x * (p_Width - 1);
		   int y_new = iuv.y * (p_Height - 1);

		   iuv.x *= (p_Width - 1);
		   iuv.y *= (p_Height - 1);

		   if ((x_new < p_Width) && (y_new < p_Height))
		   {
			   const int index_new = ((y_new * p_Width) + x_new) * 4;

			   //float4 interpCol = linInterpCol(iuv, p_Input, p_Width, p_Height);
			   float4 interpCol = {p_Input[index_new + 0], p_Input[index_new + 1], p_Input[index_new + 2], p_Input[index_new + 3]};

			   accum_col.x += interpCol.x;
			   accum_col.y += interpCol.y;
			   accum_col.z += interpCol.z;
			   accum_col.w += interpCol.w;
			}
		}
		p_Output[index + 0] = accum_col.x / samples;
		p_Output[index + 1] = accum_col.y / samples;
		p_Output[index + 2] = accum_col.z / samples;
		p_Output[index + 3] = accum_col.w / samples;
   }
}

void RunCudaKernel(int p_Width, int p_Height, float* p_Fov, float* p_Fisheye, const float* p_Input, float* p_Output, const float* p_RotMat, int p_Samples)
{
    dim3 threads(128, 1, 1);
    dim3 blocks(((p_Width + threads.x - 1) / threads.x), p_Height, 1);

	float* dev_rmat;
	cudaMalloc((void**)&dev_rmat, sizeof(float)*9*p_Samples);
	cudaMemcpy((void*)dev_rmat, (void*)p_RotMat, sizeof(float)*9*p_Samples, cudaMemcpyHostToDevice);

	float* dev_fov;
	cudaMalloc((void**)&dev_fov, sizeof(float)*p_Samples);
	cudaMemcpy((void*)dev_fov, (void*)p_Fov, sizeof(float)*p_Samples, cudaMemcpyHostToDevice);

	float* dev_fisheye;
	cudaMalloc((void**)&dev_fisheye, sizeof(float)*p_Samples);
	cudaMemcpy((void*)dev_fisheye, (void*)p_Fisheye, sizeof(float)*p_Samples, cudaMemcpyHostToDevice);

    GainAdjustKernel<<<blocks, threads>>>(p_Width, p_Height, dev_fov, dev_fisheye,
											p_Input, p_Output, dev_rmat, p_Samples);
	cudaFree( dev_rmat );
	cudaFree( dev_fov );
	cudaFree( dev_fisheye );
}
