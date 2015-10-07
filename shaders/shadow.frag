#version 330

smooth in vec2 uv;
uniform sampler2D stenciltexture;
uniform float hasstenciltexture;

void main()
{
	if(hasstenciltexture > .0f)
	{
		float alpha = textureLod(stenciltexture, uv, 0).r;
		if(alpha < .9f)
			discard;
	}
}