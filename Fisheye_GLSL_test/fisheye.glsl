#define PI 3.1415926535897932384626433832795

uniform float aperture = 3.;
uniform float pitch = 0.0;
uniform float yaw = 0.0;
uniform float fov = 1.0;
uniform float bias = 0.5;

mat3 yawMatrix(float pitch) {
	mat3 pitchMat = mat3(cos(pitch), 0, sin(pitch),
					0, 1, 0,
					-sin(pitch), 0, cos(pitch));
					
	return pitchMat;
}

mat3 pitchMatrix(float pitch) {
	mat3 pitchMat = mat3(1, 0, 0,
					0, cos(pitch), -sin(pitch),
					0, sin(pitch), cos(pitch));
					
	return pitchMat;
}

vec2 polarCoord(vec3 dir) {	
	dir = normalize(dir);
	float long = -atan(dir.z, dir.x);
	
	float lat = acos(-dir.y);
	
	vec2 uv;
	uv.x = long;
	uv.y = lat;
	
	uv /= PI;
	uv.x /= 2.0;
	uv = mod(uv, 1.0);
	return uv;
}

vec3 fisheyeDir(vec3 dir) {

	dir = dir / dir.z;
	vec2 uv = dir.xy;
	float r = sqrt(uv.x*uv.x + uv.y*uv.y);
	
	float phi = atan(uv.y, uv.x);
	
	float theta = r * aperture/2.0;
	
	vec3 fedir;
	fedir.x = sin(theta) * cos(phi);
	fedir.y = sin(theta) * sin(phi);
	fedir.z = cos(theta);
	
	mat3 yawMat = yawMatrix(yaw);
	mat3 pitchMat = pitchMatrix(pitch);
	
	fedir = pitchMat * yawMat * fedir;
	
	return fedir;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
	float aspect = iResolution.x / iResolution.y;
	


	vec3 dir;
	dir.x = (uv.x-0.5)*2.0;
	dir.y = (uv.y-0.5)*2.0;
	dir.y /= aspect;
	dir.z = fov;
	
	mat3 yawMat = yawMatrix(yaw);
	mat3 pitchMat = pitchMatrix(pitch);
	
	vec3 rectdir = pitchMat * yawMat * dir;
	//dir = normalize(dir);
	rectdir = normalize(rectdir);
	
	dir = fisheyeDir(dir) * (1.0-bias) +  rectdir * bias;

	vec2 iuv = polarCoord(dir);
	
    // Time varying pixel color
    vec3 col = texture(iChannel0, iuv).xyz;

    // Output to screen
    fragColor = vec4(col,1.0);
}