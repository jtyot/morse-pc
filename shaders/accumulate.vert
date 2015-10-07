#version 330
		
layout(location = 0) in vec2 aPosition;
smooth out vec2 uv;

void main()
{
	uv = aPosition;
	gl_Position = vec4((aPosition*2.0 - vec2(1.0)), .0, 1.0);
}