#define PI 3.1415926535897932384626433832795

float3 matMul(float16 rotMat, float3 invec){
		float3 outvec = {0, 0, 0};
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

float16 yawMatrix(float yaw) {
	float16 yawMat = {cos(yaw), 0, sin(yaw), 0, 1.0, 0, -sin(yaw), 0, cos(yaw), 0, 0, 0, 0, 0, 0, 0};
					
	return yawMat;
}

float16 pitchMatrix(float pitch) {
	float16 pitchMat = {1.0, 0, 0, 0, cos(pitch), -sin(pitch), 0, sin(pitch), cos(pitch), 0, 0, 0, 0, 0, 0, 0};
					
	return pitchMat;
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

float3 fisheyeDir(float3 dir, float16 yawMat, float16 pitchMat) {
	
	dir.x = dir.x / dir.z;
	dir.y = dir.y / dir.z;
	dir.z = dir.z / dir.z;
	
	float2 uv;
	uv.x = dir.x;
	uv.y = dir.y;
	float r = sqrt(uv.x*uv.x + uv.y*uv.y);
	
	float phi = atan2(uv.y, uv.x);
	
	float theta = r;
	
	float3 fedir;
	fedir.x = sin(theta) * cos(phi);
	fedir.y = sin(theta) * sin(phi);
	fedir.z = cos(theta);

	fedir = matMul(pitchMat, fedir);
	fedir = matMul(yawMat, fedir);
	
	return fedir;
}

__kernel void GoproVRKernel(                                       
   int p_Width,                                                       
   int p_Height,                                                      
   float p_Pitch,                                                    
   float p_Yaw,                                                  
   float p_Fov,                                                   
   float p_Fisheye,                                                     
   __global const float* p_Input,                                     
   __global float* p_Output)                      
{                                                                      
   const int x = get_global_id(0);                                  
   const int y = get_global_id(1);

   float fov = p_Fov;

	float2 uv =   {(float)x/p_Width,(float)y/p_Height};
	float aspect = (float)p_Width / (float)p_Height;

	float3 dir = {0,0,0};
	dir.x = (uv.x-0.5)*2.0;
	dir.y = (uv.y-0.5)*2.0;
	dir.y /= aspect;
	dir.z = fov;

	float16 yawMat = yawMatrix(p_Yaw);
	float16 pitchMat = pitchMatrix(-p_Pitch);
	
	float3 rectdir = matMul(pitchMat, dir);
	rectdir = matMul(yawMat, rectdir);

	rectdir = normalize(rectdir);
	
	dir = mix(rectdir, fisheyeDir(dir, yawMat, pitchMat), p_Fisheye);

	float2 iuv = polarCoord(dir);

	iuv = repairUv(iuv);

	int x_new = iuv.x * (p_Width-1);
	int y_new = iuv.y * (p_Height-1);

   if ((x_new < p_Width) && (y_new < p_Height) && (x < p_Width) && (y < p_Height))                                
   {           
			const int index = ((y * p_Width) + x) * 4;    	                                                        
       const int index_new = ((y_new * p_Width) + x_new) * 4;                      
                                                                       
       p_Output[index + 0] = p_Input[index_new + 0];   
       p_Output[index + 1] = p_Input[index_new + 1];             
       p_Output[index + 2] = p_Input[index_new + 2];             
       p_Output[index + 3] = p_Input[index_new + 3];
   }                                                                   
}
