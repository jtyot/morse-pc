#version 330

layout(location = 0) in vec3 aPosition;
uniform mat4 worldToClip;
//uniform vec4 pos;

void main()
{
	gl_Position = worldToClip * vec4(aPosition, 1.0);
}