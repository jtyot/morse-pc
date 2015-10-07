#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 auv;

uniform mat4 uModelToWorld;
uniform mat4 uWorldToClip;

smooth out vec2 uv;

void main()
{
	gl_Position = uWorldToClip * uModelToWorld * vec4(aPosition, 1.0);
	uv = auv;
}