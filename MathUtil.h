#include <glm/vec2.hpp> // vec2, bvec2, dvec2, ivec2 and uvec2
#include <glm/vec3.hpp> // vec3, bvec3, dvec3, ivec3 and uvec3
#include <glm/vec4.hpp> // vec4, bvec4, dvec4, ivec4 and uvec4
#include <glm/mat2x2.hpp> // mat2, dmat2
#include <glm/mat2x3.hpp> // mat2x3, dmat2x3
#include <glm/mat2x4.hpp> // mat2x4, dmat2x4
#include <glm/mat3x2.hpp> // mat3x2, dmat3x2
#include <glm/mat3x3.hpp> // mat3, dmat3
#include <glm/mat3x4.hpp> // mat3x4, dmat2
#include <glm/mat4x2.hpp> // mat4x2, dmat4x2
#include <glm/mat4x3.hpp> // mat4x3, dmat4x3
#include <glm/mat4x4.hpp> // mat4, dmat4
#include <glm/common.hpp> // all the GLSL common functions
#include <glm/exponential.hpp> // all the GLSL exponential functions
#include <glm/geometric.hpp> // all the GLSL geometry functions
#include <glm/integer.hpp> // all the GLSL integer functions
#include <glm/matrix.hpp> // all the GLSL matrix functions
#include <glm/packing.hpp> // all the GLSL packing functions
#include <glm/trigonometric.hpp> // all the GLSL trigonometric functions
#include <glm/vector_relational.hpp> // all the GLSL vector relational functions

using namespace glm;

static vec2 repairUv(vec2 uv){
	vec2 outuv = { 0, 0 };

	if (uv.x<0) {
		outuv.x = 1.0 + uv.x;
	}
	else if (uv.x > 1.0){
		outuv.x = uv.x - 1.0;
	}
	else {
		outuv.x = uv.x;
	}

	if (uv.y<0) {
		outuv.y = 1.0 + uv.y;
	}
	else if (uv.y > 1.0){
		outuv.y = uv.y - 1.0;
	}
	else {
		outuv.y = uv.y;
	}

	return outuv;
}

static vec2 polarCoord(vec3 dir) {
	vec3 ndir = normalize(dir);
	float longi = -atan2(ndir.z, ndir.x);

	float lat = acos(-ndir.y);

	vec2 uv;
	uv.x = longi;
	uv.y = lat;

	vec2 pitwo = vec2(M_PI, M_PI);
	uv /= pitwo;
	uv.x /= 2.0;
	vec2 ones = vec2(1.0, 1.0);
	uv = modf(uv, ones);
	return uv;
}

static vec3 fisheyeDir(vec3 dir, const mat3 rotMat) {

	dir.x = dir.x / dir.z;
	dir.y = dir.y / dir.z;
	dir.z = dir.z / dir.z;

	vec2 uv;
	uv.x = dir.x;
	uv.y = dir.y;
	float r = sqrtf(uv.x*uv.x + uv.y*uv.y);

	float phi = atan2f(uv.y, uv.x);

	float theta = r;

	vec3 fedir = { 0, 0, 0 };
	fedir.x = sin(theta) * cos(phi);
	fedir.y = sin(theta) * sin(phi);
	fedir.z = cos(theta);

	fedir = rotMat * fedir;

	return fedir;
}

vec3 tinyPlanetSph(vec3 uv) {
	vec3 sph;
	vec2 uvxy;
	uvxy.x = uv.x / uv.z;
	uvxy.y = uv.y / uv.z;

	float u = length(uvxy);
	float alpha = atan2(2.0f, u);
	float phi = M_PI - 2 * alpha;
	float z = cos(phi);
	float x = sin(phi);

	uvxy = normalize(uvxy);

	sph.z = z;

	vec2 sphxy = uvxy * x;

	sph.x = sphxy.x;
	sph.y = sphxy.y;

	return sph;
}

inline vec4 linInterpCol(vec2 uv, OFX::Image *image, OfxRectI procWindow, int width, int height){
	vec4 outCol = { 0, 0, 0, 0 };
	float i = floor(uv.x);
	float j = floor(uv.y);
	float a = uv.x - i;
	float b = uv.y - j;
	int x = procWindow.x1 + (int)i;
	int y = procWindow.y1 + (int)j;
	int x1 = (x < width - 1 ? x + 1 : x);
	int y1 = (y < width - 1 ? y + 1 : y);
	float* indexX1Y1 = static_cast<float*>(image->getPixelAddress(x, y));
	float* indexX2Y1 = static_cast<float*>(image->getPixelAddress(x1, y));
	float* indexX1Y2 = static_cast<float*>(image->getPixelAddress(x, y1));
	float* indexX2Y2 = static_cast<float*>(image->getPixelAddress(x1, y1));

	const int maxIndex = (width * height - 1) * 4;

	if ((x)* (y) < maxIndex){
		outCol.x = (1.0 - a)*(1.0 - b)*indexX1Y1[0] + a*(1.0 - b)*indexX2Y1[0] + (1.0 - a)*b*indexX1Y2[0] + a*b*indexX2Y2[0];
		outCol.y = (1.0 - a)*(1.0 - b)*indexX1Y1[1] + a*(1.0 - b)*indexX2Y1[1] + (1.0 - a)*b*indexX1Y2[1] + a*b*indexX2Y2[1];
		outCol.z = (1.0 - a)*(1.0 - b)*indexX1Y1[2] + a*(1.0 - b)*indexX2Y1[2] + (1.0 - a)*b*indexX1Y2[2] + a*b*indexX2Y2[2];
		outCol.w = (1.0 - a)*(1.0 - b)*indexX1Y1[3] + a*(1.0 - b)*indexX2Y1[3] + (1.0 - a)*b*indexX1Y2[3] + a*b*indexX2Y2[3];
	}
	else {
		outCol.x = indexX1Y1[0];
		outCol.y = indexX1Y1[1];
		outCol.z = indexX1Y1[2];
		outCol.w = indexX1Y1[3];
	}

	return outCol;
}

static float fitRange(float value, float in_min, float in_max, float out_min, float out_max){
	float out = out_min + ((out_max - out_min) / (in_max - in_min)) * (value - in_min);
	return std::min(out_max, std::max(out, out_min));
}