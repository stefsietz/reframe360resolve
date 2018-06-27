#define PI 3.1415926535897932384626433832795

uniform float aperture = 3.;
uniform float pitch = 0.0;
uniform float yaw = 0.0;
uniform float fov = 1.0;
uniform float planetdist = 1.0;
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



vec3 tinyPlanetSph(vec2 uv, float dist) {
	vec3 sph;
	float u  =length(uv);
	float fact = dist*2.0*u/(u*u+dist*dist);
	float z = (u*u - dist*dist)/(u*u+dist*dist);
	sph.xy = uv * fact;
	
	if(fact < 1.0){
		sph.xy /= fact/2.0;
		z = z/4.0;
	}
	sph.z = z;
	return sph;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
	float aspect = iResolution.x / iResolution.y;
	
	uv = (uv - vec2(0.5)) * 2.0;
	uv.y /= aspect;

	uv /= fov;
	
	vec3 dir = vec3(uv, 1.0);
	dir = normalize(dir);
	
	mat3 yawMat = yawMatrix(yaw);
	mat3 pitchMat = pitchMatrix(pitch);
	
	dir= pitchMat * yawMat * dir;

	vec2 iuv = polarCoord(dir);
	
    // Time varying pixel color
    vec3 col = texture(iChannel0, iuv).xyz;

    // Output to screen
    fragColor = vec4(col,1.0);
}