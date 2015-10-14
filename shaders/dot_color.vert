#version 330

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 worldToClip;
//uniform vec4 pos;
out vec3 col;

void main()
{
	col = aColor;
	gl_Position = worldToClip * vec4(aPosition, 1.0);
}