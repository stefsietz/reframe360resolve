#define PI 3.1415926535897932384626433832795

float3 matMul(float16 rotMat, float3 invec){
	float3 outvec = { 0, 0, 0 };
	outvec.x = dot(rotMat.s012, invec);
	outvec.y = dot(rotMat.s345, invec);
	outvec.z = dot(rotMat.s678, invec);
	return outvec;
}


float2 repairUv(float2 uv){
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

float2 polarCoord(float3 dir) {	
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
	uv = fmod(uv, ones);
	return uv;
}

float3 fisheyeDir(float3 dir, float16 rotMat) {
	
	dir.x = dir.x / dir.z;
	dir.y = dir.y / dir.z;
	dir.z = dir.z / dir.z;
	
	float2 uv;
	uv.x = dir.x;
	uv.y = dir.y;
	float r = sqrt(uv.x*uv.x + uv.y*uv.y);
	
	float phi = atan2(uv.y, uv.x);
	
	float theta = r;
	
	float3 fedir = { 0, 0, 0 };
	fedir.x = sin(theta) * cos(phi);
	fedir.y = sin(theta) * sin(phi);
	fedir.z = cos(theta);

	fedir = matMul(rotMat, fedir);
	
	return fedir;
}

float4 linInterpCol(float2 uv, __global const float* input, int width, int height){
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

__kernel void GoproVRKernel(                                       
	int p_Width, int p_Height, __global float* p_Fov, __global float* p_Fisheye,
	__global const float* p_Input, __global float* p_Output, __global float* r, int samples, bool bilinear)
{                                                                      
   const int x = get_global_id(0);                                  
   const int y = get_global_id(1);

   if ((x < p_Width) && (y < p_Height)){
	   const int index = ((y * p_Width) + x) * 4;

	   float4 accum_col = { 0, 0, 0, 0 };

	   for (int i = 0; i < samples; i++){

		   float fov = p_Fov[i];

		   float2 uv = { (float)x / p_Width, (float)y / p_Height };
		   float aspect = (float)p_Width / (float)p_Height;

		   float3 dir = { 0, 0, 0 };
		   dir.x = (uv.x - 0.5)*2.0;
		   dir.y = (uv.y - 0.5)*2.0;
		   dir.y /= aspect;
		   dir.z = fov;

		   float16 rotMat = { r[i * 9 + 0], r[i * 9 + 1], r[i * 9 + 2],
			   r[i * 9 + 3], r[i * 9 + 4], r[i * 9 + 5],
			   r[i * 9 + 6], r[i * 9 + 7], r[i * 9 + 8], 0, 0, 0, 0, 0, 0, 0 };

		   float3 rectdir = dir;
		   rectdir = matMul(rotMat, dir);

		   rectdir = normalize(rectdir);

		   dir = mix(rectdir, fisheyeDir(dir, rotMat), p_Fisheye[i]);

		   float2 iuv = polarCoord(dir);

		   iuv = repairUv(iuv);

		   int x_new = iuv.x * (p_Width - 1);
		   int y_new = iuv.y * (p_Height - 1);

		   iuv.x *= (p_Width - 1);
		   iuv.y *= (p_Height - 1);

		   if ((x_new < p_Width) && (y_new < p_Height))
		   {
			   const int index_new = ((y_new * p_Width) + x_new) * 4;

			   float4 interpCol;
			   if (bilinear){
				   interpCol = linInterpCol(iuv, p_Input, p_Width, p_Height);
			   }
			   else {
				   interpCol = { p_Input[index_new + 0], p_Input[index_new + 1], p_Input[index_new + 2], p_Input[index_new + 3] };
			   }

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
