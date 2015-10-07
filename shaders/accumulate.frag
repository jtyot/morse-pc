#version 330

smooth in vec2 uv;
uniform sampler2D tex;
uniform float filter_weight;
out layout(location=0) vec4 fColor;
out layout(location=1) float weight;

void main()
{
	fColor = vec4(texture(tex, uv).rgb * filter_weight, 1.0);
	weight = filter_weight;
}