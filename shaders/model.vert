#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
		
smooth out vec3 normal;
smooth out vec3 worldpos;
smooth out vec2 uv;
smooth out mat3 tangent_basis;
out vec3 lightdir;

uniform mat4 uModelToWorld;
uniform mat4 uWorldToClip;
uniform mat4 uNormalTransform;
uniform float uShading;
		

const vec3 distinctColors[6] = vec3[6](
	vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
	vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0));
const vec3 directionToLight = normalize(vec3(0.5, 0.5, -0.6));
		
void main()
{
	normal = normalize((uNormalTransform*vec4(aNormal, .0)).xyz);
	vec3 tangent = normalize(aTangent);
	vec3 bitangent = normalize(aBitangent);
	tangent_basis = mat3(tangent, bitangent, normal);
	worldpos = (uModelToWorld*vec4(aPosition, 1.0)).xyz;
	gl_Position = uWorldToClip * uModelToWorld * vec4(aPosition, 1.0);
	lightdir = directionToLight;
	uv = aUV;
}