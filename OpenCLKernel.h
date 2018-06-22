const char *KernelSource = "\n" \
"#define PI 3.1415926535897932384626433832795\n" \
"\n" \
"float3 matMul(float16 rotMat, float3 invec){\n" \
"float3 outvec = {0, 0, 0};\n" \
"outvec.x = dot(rotMat.s012, invec);\n" \
"outvec.y = dot(rotMat.s345, invec);\n" \
"outvec.z = dot(rotMat.s678, invec);\n" \
"return outvec;\n" \
"}\n" \
"\n" \
"float2 repairUv(float2 uv){\n" \
"float2 outuv = {0, 0};\n" \
"\n" \
"if(uv.x<0) {\n" \
"outuv.x = 1.0 + uv.x;\n" \
"}else if(uv.x > 1.0){\n" \
"outuv.x = uv.x -1.0;\n" \
"} else {\n" \
"outuv.x = uv.x;\n" \
"}\n" \
"\n" \
"if(uv.y<0) {\n" \
"outuv.y = 1.0 + uv.y;\n" \
"} else if(uv.y > 1.0){\n" \
"outuv.y = uv.y -1.0;\n" \
"} else {\n" \
"outuv.y = uv.y;\n" \
"}\n" \
"\n" \
"return outuv;\n" \
"}\n" \
"\n" \
"float16 yawMatrix(float yaw) {\n" \
"float16 yawMat = {cos(yaw), 0, sin(yaw), 0, 1.0, 0, -sin(yaw), 0, cos(yaw), 0, 0, 0, 0, 0, 0, 0};\n" \
"\n" \
"return yawMat;\n" \
"}\n" \
"\n" \
"float16 pitchMatrix(float pitch) {\n" \
"float16 pitchMat = {1.0, 0, 0, 0, cos(pitch), -sin(pitch), 0, sin(pitch), cos(pitch), 0, 0, 0, 0, 0, 0, 0};\n" \
"\n" \
"return pitchMat;\n" \
"}\n" \
"\n" \
"float2 polarCoord(float3 dir) {\n" \
"float3 ndir = normalize(dir);\n" \
"float longi = -atan2(ndir.z, ndir.x);\n" \
"\n" \
"float lat = acos(-ndir.y);\n" \
"\n" \
"float2 uv;\n" \
"uv.x = longi;\n" \
"uv.y = lat;\n" \
"\n" \
"float2 pitwo = {PI, PI};\n" \
"uv /= pitwo;\n" \
"uv.x /= 2.0;\n" \
"float2 ones = {1.0, 1.0};\n" \
"uv = fmod(uv, ones);\n" \
"return uv;\n" \
"}\n" \
"\n" \
"float3 fisheyeDir(float3 dir, float16 yawMat, float16 pitchMat) {\n" \
"\n" \
"dir.x = dir.x / dir.z;\n" \
"dir.y = dir.y / dir.z;\n" \
"dir.z = dir.z / dir.z;\n" \
"\n" \
"float2 uv;\n" \
"uv.x = dir.x;\n" \
"uv.y = dir.y;\n" \
"float r = sqrt(uv.x*uv.x + uv.y*uv.y);\n" \
"\n" \
"float phi = atan2(uv.y, uv.x);\n" \
"\n" \
"float theta = r;\n" \
"\n" \
"float3 fedir;\n" \
"fedir.x = sin(theta) * cos(phi);\n" \
"fedir.y = sin(theta) * sin(phi);\n" \
"fedir.z = cos(theta);\n" \
"\n" \
"fedir = matMul(pitchMat, fedir);\n" \
"fedir = matMul(yawMat, fedir);\n" \
"\n" \
"return fedir;\n" \
"}\n" \
"\n" \
"__kernel void GoproVRKernel(\n" \
"int p_Width,\n" \
"int p_Height,\n" \
"float p_Pitch,\n" \
"float p_Yaw,\n" \
"float p_Fov,\n" \
"float p_Fisheye,\n" \
"__global const float* p_Input,\n" \
"__global float* p_Output)\n" \
"{\n" \
"const int x = get_global_id(0);\n" \
"const int y = get_global_id(1);\n" \
"\n" \
"float fov = p_Fov;\n" \
"\n" \
"float2 uv =   {(float)x/p_Width,(float)y/p_Height};\n" \
"float aspect = (float)p_Width / (float)p_Height;\n" \
"\n" \
"float3 dir = {0,0,0};\n" \
"dir.x = (uv.x-0.5)*2.0;\n" \
"dir.y = (uv.y-0.5)*2.0;\n" \
"dir.y /= aspect;\n" \
"dir.z = fov;\n" \
"\n" \
"float16 yawMat = yawMatrix(p_Yaw);\n" \
"float16 pitchMat = pitchMatrix(-p_Pitch);\n" \
"\n" \
"float3 rectdir = matMul(pitchMat, dir);\n" \
"rectdir = matMul(yawMat, rectdir);\n" \
"\n" \
"rectdir = normalize(rectdir);\n" \
"\n" \
"dir = mix(rectdir, fisheyeDir(dir, yawMat, pitchMat), p_Fisheye);\n" \
"\n" \
"float2 iuv = polarCoord(dir);\n" \
"\n" \
"iuv = repairUv(iuv);\n" \
"\n" \
"int x_new = iuv.x * (p_Width-1);\n" \
"int y_new = iuv.y * (p_Height-1);\n" \
"\n" \
"if ((x_new < p_Width) && (y_new < p_Height) && (x < p_Width) && (y < p_Height))\n" \
"{\n" \
"const int index = ((y * p_Width) + x) * 4;\n" \
"const int index_new = ((y_new * p_Width) + x_new) * 4;\n" \
"\n" \
"p_Output[index + 0] = p_Input[index_new + 0];\n" \
"p_Output[index + 1] = p_Input[index_new + 1];\n" \
"p_Output[index + 2] = p_Input[index_new + 2];\n" \
"p_Output[index + 3] = p_Input[index_new + 3];\n" \
"}\n" \
"}\n" \
"\n";