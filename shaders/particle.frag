#version 330

uniform vec3 campos;
uniform vec2 screen;
uniform vec3 col;

uniform sampler2DArray shadowmap;
uniform vec3 lightpos[20];
uniform mat4 lightProject[20];
uniform vec3 lightforward[20];
uniform float lightfov[20];
uniform vec3 lightcol[20];
uniform vec3 ambient_light;
uniform float shadowmode;
uniform int lightsamples;

out vec4 fColor;
smooth in vec3 acol;
smooth in vec3 worldpos;
void main()
{
	fColor.rgb = acol;
	if(shadowmode < 1.0)
	{
		for(int i = 0; i < lightsamples; i++)
		{
			vec4 posLightClip = (lightProject[i] * vec4(worldpos.xyz, 1.0));
			vec2 light_uv = posLightClip.xy;
			vec3 pos_NDC = (posLightClip.xyz / posLightClip.w + vec3(1.0)) * .5;
			if(textureLod(shadowmap, vec3(pos_NDC.xy, float(i)), 0).r >= pos_NDC.z)
			{
				vec3 lightdir = normalize(lightpos[i] - worldpos.xyz);
				float lightang = clamp((max(dot(-lightdir, lightforward[i]), .0) - cos(lightfov[i] * .5 * 3.1415 / 180.0)) * 5.0, .0, 1.0);
				vec3 light = lightcol[i] / max(dot(lightpos[i] - worldpos.xyz, lightpos[i] - worldpos.xyz), .02);
				fColor.rgb += lightang * light * col;
			}
		}
		fColor.rgb += ambient_light * col;
	}
	fColor.a = 1.0;
}