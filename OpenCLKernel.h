const char *KernelSource = "\n" \
"#define PI 3.1415926535897932384626433832795\n" \
"\n" \
"float3 matMul(float16 rotMat, float3 invec){\n" \
"float3 outvec = { 0, 0, 0 };\n" \
"outvec.x = dot(rotMat.s012, invec);\n" \
"outvec.y = dot(rotMat.s345, invec);\n" \
"outvec.z = dot(rotMat.s678, invec);\n" \
"return outvec;\n" \
"}\n" \
"\n" \
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
"float3 fisheyeDir(float3 dir, float16 rotMat) {\n" \
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
"float3 fedir = { 0, 0, 0 };\n" \
"fedir.x = sin(theta) * cos(phi);\n" \
"fedir.y = sin(theta) * sin(phi);\n" \
"fedir.z = cos(theta);\n" \
"\n" \
"fedir = matMul(rotMat, fedir);\n" \
"\n" \
"return fedir;\n" \
"}\n" \
"\n" \
"float4 linInterpCol(float2 uv, __global const float* input, int width, int height){\n" \
"float4 outCol = {0,0,0,0};\n" \
"float i = floor(uv.x);\n" \
"float j = floor(uv.y);\n" \
"float a = uv.x-i;\n" \
"float b = uv.y-j;\n" \
"int x = (int)i;\n" \
"int y = (int)j;\n" \
"const int indexX1Y1 = ((y * width) + x) * 4;\n" \
"const int indexX2Y1 = ((y * width) + x+1) * 4;\n" \
"const int indexX1Y2 = (((y+1) * width) + x) * 4;\n" \
"const int indexX2Y2 = (((y+1) * width) + x+1) * 4;\n" \
"const int maxIndex = (width * height -1) * 4;\n" \
"\n" \
"if(indexX2Y2 < maxIndex-height - 100){\n" \
"outCol.x = (1.0 - a)*(1.0 - b)*input[indexX1Y1] + a*(1.0 - b)*input[indexX2Y1] + (1.0 - a)*b*input[indexX1Y2] + a*b*input[indexX2Y2];\n" \
"outCol.y = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 1] + a*(1.0 - b)*input[indexX2Y1 + 1] + (1.0 - a)*b*input[indexX1Y2 + 1] + a*b*input[indexX2Y2 + 1];\n" \
"outCol.z = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 2] + a*(1.0 - b)*input[indexX2Y1 + 2] + (1.0 - a)*b*input[indexX1Y2 + 2] + a*b*input[indexX2Y2 + 2];\n" \
"outCol.w = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 3] + a*(1.0 - b)*input[indexX2Y1 + 3] + (1.0 - a)*b*input[indexX1Y2 + 3] + a*b*input[indexX2Y2 + 3];\n" \
"} else {\n" \
"outCol.x = input[indexX1Y1];\n" \
"outCol.y = input[indexX1Y1+ 1];\n" \
"outCol.z = input[indexX1Y1+ 2];\n" \
"outCol.w = input[indexX1Y1+ 3];\n" \
"}\n" \
"return outCol;\n" \
"}\n" \
"\n" \
"__kernel void GoproVRKernel(\n" \
"int p_Width,\n" \
"int p_Height,\n" \
"float p_Pitch,\n" \
"float p_Yaw,\n" \
"float p_Fov,\n" \
"float p_Fisheye,\n" \
"float r0, float r1, float r2, float r3, float r4, float r5, float r6, float r7, float r8,\n" \
"__global const float* p_Input,\n" \
"__global float* p_Output)\n" \
"{\n" \
"const int x = get_global_id(0);\n" \
"const int y = get_global_id(1);\n" \
"\n" \
"if ((x < p_Width) && (y < p_Height)){\n" \
"float fov = p_Fov;\n" \
"\n" \
"float2 uv = { (float)x / p_Width, (float)y / p_Height };\n" \
"float aspect = (float)p_Width / (float)p_Height;\n" \
"\n" \
"float3 dir = { 0, 0, 0 };\n" \
"dir.x = (uv.x - 0.5)*2.0;\n" \
"dir.y = (uv.y - 0.5)*2.0;\n" \
"dir.y /= aspect;\n" \
"dir.z = fov;\n" \
"\n" \
"float16 rotMat = { r0, r1, r2, r3, r4, r5, r6, r7, r8, 0, 0, 0, 0, 0, 0, 0 };\n" \
"\n" \
"float3 rectdir = dir;\n" \
"rectdir = matMul(rotMat, dir);\n" \
"\n" \
"rectdir = normalize(rectdir);\n" \
"\n" \
"dir = mix(rectdir, fisheyeDir(dir, rotMat), p_Fisheye);\n" \
"\n" \
"float2 iuv = polarCoord(dir);\n" \
"\n" \
"iuv = repairUv(iuv);\n" \
"\n" \
"int x_new = iuv.x * (p_Width - 1);\n" \
"int y_new = iuv.y * (p_Height - 1);\n" \
"\n" \
"iuv.x *= (p_Width - 1);\n" \
"iuv.y *= (p_Height - 1);\n" \
"\n" \
"if ((x_new < p_Width) && (y_new < p_Height))\n" \
"{\n" \
"const int index = ((y * p_Width) + x) * 4;\n" \
"const int index_new = ((y_new * p_Width) + x_new) * 4;\n" \
"\n" \
"float4 interpCol = linInterpCol(iuv, p_Input, p_Width, p_Height);\n" \
"\n" \
"p_Output[index + 0] = interpCol.x;\n" \
"p_Output[index + 1] = interpCol.y;\n" \
"p_Output[index + 2] = interpCol.z;\n" \
"p_Output[index + 3] = interpCol.w;\n" \
"}\n" \
"}\n" \
"}\n" \
"\n";