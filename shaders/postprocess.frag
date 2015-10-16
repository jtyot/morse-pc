#version 330

smooth in vec2 uv_in;
uniform sampler2D tex;
uniform sampler2D weight;
uniform float time;
uniform int greyscale;
uniform int use_weight;
uniform vec2 flip_y;
uniform vec2 screen;
uniform vec2 projectionoffset;
uniform float shadowCutOff;
out vec4 fColor;

vec3 tonemap(vec3 col, float toneMapWhite)
{
	float L = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
	float nL = L * (1.0 + L / toneMapWhite / toneMapWhite) / (1.0 + L);
	float scale = nL / L;
	return col * scale;
}

void main()
{
	vec2 uv = (uv_in * 2.0 - .5) + projectionoffset;
	if(flip_y.y < .0)
		uv = vec2(uv_in.x, uv_in.y);
	if(uv.x < .0 || uv.y < .0 || uv.x > 1.0 || uv.y > 1.0)
		fColor = vec4(.0, .0, .0, 1.0);
	else
	{
		fColor = vec4(texture(tex, uv).rgb, 1.0);

		if(greyscale == 1)
			fColor.rgb = fColor.bbb;

		if(use_weight == 1)
		{
			fColor.rgb = max(vec3(.0f), fColor.rgb - vec3(shadowCutOff)) / (1.0f - shadowCutOff);

			fColor.rgb = tonemap(fColor.rgb, 100.0);
		}
		fColor.a = 1.0;
	}
}