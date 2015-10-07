#version 330

smooth in vec2 uv_in;
uniform sampler2D tex;
uniform sampler2D weight;
uniform float time;
uniform vec2 flip_y;
uniform vec2 screen;
uniform vec2 projectionoffset;
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
		float len = length(texture(tex, uv).rgb);
		if(len > .0)
		{
			if(flip_y.y > .0)
				fColor = vec4(texture(tex, uv).rgb / texture(weight, uv).r, 1.0);
			else
				fColor = vec4(texture(tex, uv).rgb, 1.0);
		}
		else
			fColor.rgb = vec3(.0);

		fColor.rgb = tonemap(fColor.rgb, 100.0);
		fColor.a = 1.0;
	}
}