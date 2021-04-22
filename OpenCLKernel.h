const char *KernelSource = "\n" \
"#define PI 3.1415926535897932384626433832795\n" \
"\n" \
"float3 matMul(float16 rotMat, float3 invec){\n" \
"float3 outvec;\n" \
"outvec.x = dot(rotMat.s012, invec);\n" \
"outvec.y = dot(rotMat.s345, invec);\n" \
"outvec.z = dot(rotMat.s678, invec);\n" \
"return outvec;\n" \
"}\n" \
"\n" \
"\n" \
"float2 repairUv(float2 uv){\n" \
"float2 outuv = { 0, 0 };\n" \
"\n" \
"if (uv.x<0) {\n" \
"outuv.x = 1.0 + uv.x;\n" \
"}\n" \
"else if (uv.x > 1.0){\n" \
"outuv.x = uv.x - 1.0;\n" \
"}\n" \
"else {\n" \
"outuv.x = uv.x;\n" \
"}\n" \
"\n" \
"if (uv.y<0) {\n" \
"outuv.y = 1.0 + uv.y;\n" \
"}\n" \
"else if (uv.y > 1.0){\n" \
"outuv.y = uv.y - 1.0;\n" \
"}\n" \
"else {\n" \
"outuv.y = uv.y;\n" \
"}\n" \
"\n" \
"outuv.x = min(max(outuv.x, 0.0f), 1.0f);\n" \
"outuv.y = min(max(outuv.y, 0.0f), 1.0f);\n" \
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
"float2 pitwo = { PI, PI };\n" \
"uv /= pitwo;\n" \
"uv.x /= 2.0;\n" \
"float2 ones = { 1.0, 1.0 };\n" \
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
"float3 tinyPlanetSph(float3 uv) {\n" \
"float3 sph;\n" \
"float2 uvxy;\n" \
"uvxy.x = uv.x / uv.z;\n" \
"uvxy.y = uv.y / uv.z;\n" \
"\n" \
"float u = length(uvxy);\n" \
"float alpha = atan2(2.0f, u);\n" \
"float phi = PI - 2 * alpha;\n" \
"float z = cos(phi);\n" \
"float x = sin(phi);\n" \
"\n" \
"uvxy = normalize(uvxy);\n" \
"\n" \
"sph.z = z;\n" \
"\n" \
"float2 sphxy;\n" \
"sphxy.x = uvxy.x * x;\n" \
"sphxy.y = uvxy.y * x;\n" \
"\n" \
"sph.x = sphxy.x;\n" \
"sph.y = sphxy.y;\n" \
"\n" \
"return sph;\n" \
"}\n" \
"\n" \
"float4 linInterpCol(float2 uv, __global const float* input, int width, int height){\n" \
"float4 outCol;\n" \
"float i = floor(uv.x);\n" \
"float j = floor(uv.y);\n" \
"float a = uv.x - i;\n" \
"float b = uv.y - j;\n" \
"int x = (int)i;\n" \
"int y = (int)j;\n" \
"int x1 = (x < width - 1 ? x + 1 : x);\n" \
"int y1 = (y < height - 1 ? y + 1 : y);\n" \
"const int indexX1Y1 = ((y * width) + x) * 4;\n" \
"const int indexX2Y1 = ((y * width) + x1) * 4;\n" \
"const int indexX1Y2 = (((y1) * width) + x) * 4;\n" \
"const int indexX2Y2 = (((y1) * width) + x1) * 4;\n" \
"const int maxIndex = (width * height - 1) * 4;\n" \
"\n" \
"if (indexX2Y2 < maxIndex){\n" \
"outCol.x = (1.0 - a)*(1.0 - b)*input[indexX1Y1] + a*(1.0 - b)*input[indexX2Y1] + (1.0 - a)*b*input[indexX1Y2] + a*b*input[indexX2Y2];\n" \
"outCol.y = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 1] + a*(1.0 - b)*input[indexX2Y1 + 1] + (1.0 - a)*b*input[indexX1Y2 + 1] + a*b*input[indexX2Y2 + 1];\n" \
"outCol.z = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 2] + a*(1.0 - b)*input[indexX2Y1 + 2] + (1.0 - a)*b*input[indexX1Y2 + 2] + a*b*input[indexX2Y2 + 2];\n" \
"outCol.w = (1.0 - a)*(1.0 - b)*input[indexX1Y1 + 3] + a*(1.0 - b)*input[indexX2Y1 + 3] + (1.0 - a)*b*input[indexX1Y2 + 3] + a*b*input[indexX2Y2 + 3];\n" \
"}\n" \
"else {\n" \
"outCol.x = input[indexX1Y1];\n" \
"outCol.y = input[indexX1Y1 + 1];\n" \
"outCol.z = input[indexX1Y1 + 2];\n" \
"outCol.w = input[indexX1Y1 + 3];\n" \
"}\n" \
"return outCol;\n" \
"}\n" \
"\n" \
"__kernel void Reframe360Kernel(\n" \
"int p_Width, int p_Height, __global float* p_Fov, __global float* p_Tinyplanet, __global float* p_Rectilinear,\n" \
"__global const float* p_Input, __global float* p_Output, __global float* r, int samples, int bilinear)\n" \
"{\n" \
"const int x = get_global_id(0);\n" \
"const int y = get_global_id(1);\n" \
"\n" \
"if ((x < p_Width) && (y < p_Height)){\n" \
"const int index = ((y * p_Width) + x) * 4;\n" \
"\n" \
"float4 accum_col = { 0, 0, 0, 0 };\n" \
"\n" \
"for (int i = 0; i < samples; i++){\n" \
"\n" \
"float fov = p_Fov[i];\n" \
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
"float3 tinyplanet = tinyPlanetSph(dir);\n" \
"tinyplanet = normalize(tinyplanet);\n" \
"\n" \
"float16 rotMat = { r[i * 9 + 0], r[i * 9 + 1], r[i * 9 + 2],\n" \
"r[i * 9 + 3], r[i * 9 + 4], r[i * 9 + 5],\n" \
"r[i * 9 + 6], r[i * 9 + 7], r[i * 9 + 8], 0, 0, 0, 0, 0, 0, 0 };\n" \
"\n" \
"tinyplanet = matMul(rotMat, tinyplanet);\n" \
"float3 rectdir = matMul(rotMat, dir);\n" \
"\n" \
"rectdir = normalize(rectdir);\n" \
"\n" \
"dir = mix(fisheyeDir(dir, rotMat), tinyplanet, p_Tinyplanet[i]);\n" \
"dir = mix(dir, rectdir, p_Rectilinear[i]);\n" \
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
"const int index_new = ((y_new * p_Width) + x_new) * 4;\n" \
"\n" \
"float4 interpCol;\n" \
"if (bilinear){\n" \
"interpCol = linInterpCol(iuv, p_Input, p_Width, p_Height);\n" \
"}\n" \
"else {\n" \
"interpCol = (float4)( p_Input[index_new + 0], p_Input[index_new + 1], p_Input[index_new + 2], p_Input[index_new + 3] );\n" \
"}\n" \
"\n" \
"accum_col.x += interpCol.x;\n" \
"accum_col.y += interpCol.y;\n" \
"accum_col.z += interpCol.z;\n" \
"accum_col.w += interpCol.w;\n" \
"}\n" \
"}\n" \
"p_Output[index + 0] = accum_col.x / samples;\n" \
"p_Output[index + 1] = accum_col.y / samples;\n" \
"p_Output[index + 2] = accum_col.z / samples;\n" \
"p_Output[index + 3] = accum_col.w / samples;\n" \
"}\n" \
"}\n" \
"\n";
