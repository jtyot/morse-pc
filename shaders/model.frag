#version 330
smooth in vec3 normal;
smooth in vec3 worldpos;
smooth in vec2 uv;
smooth in mat3 tangent_basis;
in float clampedCosine;
in vec3 lightdir;

uniform vec3 campos;
uniform sampler2DArray shadowmap;

uniform float hasenvimap;
uniform sampler2D envimap;

uniform float hasalbedotex;
uniform sampler2D albedotex;

uniform float hasspeculartex;
uniform sampler2D speculartex;

uniform float hasemissiontex;
uniform sampler2D emissiontex;

uniform float hasbumpmap;
uniform float bumpmap_objectspace;
uniform sampler2D bumpmap;
uniform mat4 normal_transform;

uniform float hasstenciltexture;
uniform sampler2D stenciltexture;

uniform float time;
uniform float uShading;

uniform float specular_col_mult;
uniform float specular_exp_mult;
uniform float specular_col_pow;
uniform float specular_exp_pow;

uniform vec4 col_alpha;
uniform vec4 specular_glossiness;
uniform vec4 emission_padding;
uniform vec3 lightpos[10];
uniform mat4 lightProject[10];
uniform vec3 lightforward[10];
uniform float lightfov[10];
uniform vec3 lightcol[10];
uniform int lightsamples;
uniform vec3 ambient_light;
uniform float light_min_dist;

const float p_phong = .2;
const float p_diffuse = .8;
/*const float exp = 21330.0;
const vec3 diffusecol = vec3(.6, 1.0, .8)*1.25;
const vec3 specularcol = vec3(1.0, 1.0, 1.0)*7.0;*/
const float supersamples = 1.0;

/*float rnd(vec2 p, float s)
{
	return fract(1235.9*cos(1.9*dot(vec3(p, s), vec3(19.7, 17.5, 3.3))));
}

vec3 picksample(vec3 indir, vec3 n, vec2 seed1, float seed2, float seed3, mat3 specular_basis, mat3 diffuse_basis, float exp)
{
	vec3 dir;
	if (seed3 < p_phong)//get specular sample
	{
		float u1 = rnd(seed1*123.0, seed2*12.5);
		float u2 = rnd(seed1, seed2);
		float theta = acos(pow(u1, 1.0f / (exp + 1)));
		float phi = 2.0 * 3.1415926 * u2;
		dir.x = sin(theta)*cos(phi);
		dir.y = sin(theta)*sin(phi);
		dir.z = cos(theta);
		return specular_basis*dir;
	}
	else// if(seed3 < p_phong)//get diffuse sample
	{
		float u1 = rnd(seed1*vec2(1.15, 1.21), seed2*12.5);
		float u2 = rnd(seed1*vec2(2.11, 2.15), seed2*2.5);
		float theta = acos(sqrt(u1));
		float phi = 2.0 * 3.1415926 * u2;
		dir.x = sin(theta)*cos(phi);
		dir.y = sin(theta)*sin(phi);
		dir.z = cos(theta);
		return diffuse_basis*dir;
	}
}

float getpdf(vec3 n, vec3 outdir, vec3 dirac_outdir, float exp)
{
	float cosa = max(dot(outdir, n), .0);
	float cosa_mirror = max(dot(outdir, dirac_outdir), .0f);
	float diffuse_pdf = .0, phong_pdf = .0;
	diffuse_pdf = 1 / 3.1415926*cosa*p_diffuse;			//probability from diffuse sample
	phong_pdf = (exp + 1) / 2 / 3.1415926*pow(cosa_mirror, exp)*p_phong;	//probability from phong sample
	return diffuse_pdf + phong_pdf + .1 / 3.1415;
}*/

vec3 evaluatebrdf(float pdf, float cosa, float cosa_mirror, vec3 outdir, vec3 indir, vec3 diffusecol, vec3 specularcol, float exp)
{
	if(pdf == .0 || dot(normal, outdir) < .0)
		return vec3(.0);
	//calculate fresnel with schlick's approximation:
	vec3 H = (indir + outdir)*.5;
	float n1 = 1.0;
	float n2 = 1.7;
	float R0 = pow((n1-n2)/(n1+n2), 2.0);
	float R = R0 + (1-R0)*pow(clamp(dot(H, indir), .0, 1.0), 5.0);
	return ((exp + 2.0) / (2.0 * 3.1415926)*pow(cosa_mirror, exp)*R*specularcol + cosa*diffusecol / 3.1415926) / pdf;
}

mat3 formbasis(vec3 n)
{
	vec3 v2;
	vec3 v3;
	if (abs(n.x) > abs(n.y))
	{
		float invlen = 1.0 / sqrt(n.x*n.x + n.z*n.z);
		v2 = vec3(-n.z*invlen, .0, n.x*invlen);
	}
	else
	{
		float invlen = 1.0 / sqrt(n.y*n.y + n.z*n.z);
		v2 = vec3(.0, n.z*invlen, -n.y*invlen);
	}
	v3 = cross(n, v2);
	mat3 R = mat3(v3, v2, n);
	return R;
}

out vec4 fColor;
vec3 anormal = normalize(normal);
const vec3 directionToLight = normalize(vec3(0.5, 0.5, -0.6));

void main()
{
	//calculate necessary vectors
	float alpha = 1.0;
	if(hasstenciltexture > .0f)
	{
		alpha = textureLod(stenciltexture, uv, 0).r;
		if(alpha < .9)
			discard;
	}
	mat3 diffuse_basis = formbasis(anormal);
	vec3 n = anormal;
	if(hasbumpmap > .0f)
	{
		if(bumpmap_objectspace > .0f)
			n = texture(bumpmap, uv).rgb * 2.0 - vec3(1.0);
		else
			n = normalize(tangent_basis * (texture(bumpmap, uv).rgb * 2.0 - vec3(1.0)));
		diffuse_basis = formbasis(n);
	}
	n = (normal_transform * vec4(n, .0f)).xyz;

	vec3 indir = normalize(worldpos-campos);
	vec3 dirac_outdir = normalize(indir - n * 2.0 * dot(n, indir));

	mat3 specular_basis = formbasis(dirac_outdir);
	fColor = vec4(.0);
	vec3 diffusecol, specularcol;
	float exp;
	diffusecol = col_alpha.rgb;
	if(hasalbedotex > .0f)
		diffusecol = texture(albedotex, uv).rgb;
	if(hasspeculartex > .0f)
	{
		exp = max(pow(length(texture(speculartex, uv).rgb), specular_exp_pow) * 100.0, .01) * specular_exp_mult;
		float L = length(diffusecol);
		specularcol = diffusecol * pow(L, specular_col_pow);
	}
	else
	{
		specularcol = specular_glossiness.rgb;
		exp = specular_glossiness.w;
	}
	//do a supersampling loop to get better quality
	/*for (float i = 1.0; i <= supersamples; i++)
	{
		float seed = worldpos.x*1.02 + time*1.06 + worldpos.z*1.0 + i*3.0;
		vec2 seed2 = vec2(seed*1.12+worldpos.x*1.1 + i*2.0, seed*2.2+worldpos.y*4.1 + i*4.2);
		float seed3 = rnd(seed2*vec2(12.02, 102.5), seed*120.2 + i*12.0 + time*1.0);
		vec3 outdir = picksample(indir, n, seed2, seed, seed3, specular_basis, diffuse_basis, exp);
		float pdf = getpdf(n, outdir, dirac_outdir, exp);
		float cosa = clamp(dot(outdir, n), .0, 1.0);
		float cosa_mirror = clamp(dot(outdir, dirac_outdir), .0, 1.0);

		//read envimap
		float u = acos(outdir.y) / 3.141;
		float v = atan(outdir.z, outdir.x) / 6.283;
		vec3 envcol = textureLod(envimap, vec2(v, u), 0).xyz;
		float L = dot(vec3(0.2126, 0.7152, 0.0722), envcol);
		envcol = 3.0 * envcol * pow(L, 3.0);
		//fColor += .5*vec3(1.0, .9, .8) * clamp(dot(n, lightdir), .0, 1.0);

		fColor.rgb += envcol * evaluatebrdf(pdf, cosa, cosa_mirror, outdir, indir, diffusecol, specularcol, exp) / supersamples;
	}*/
	for(int i = 0; i < lightsamples; i++)
	{
		vec4 posLightClip = (lightProject[i] * vec4(worldpos, 1.0));
		vec2 light_uv = posLightClip.xy;
		vec3 pos_NDC = (posLightClip.xyz / posLightClip.w + vec3(1.0)) * .5;
		if(textureLod(shadowmap, vec3(pos_NDC.xy, float(i)), 0).r > pos_NDC.z)
		{
			vec3 lightdir = normalize(lightpos[i] - worldpos);
			float cosa_light = clamp(dot(lightdir, n), .0, 1.0);
			float cosa_mirrorlight = clamp(dot(lightdir, dirac_outdir), .0, 1.0);
			float lightang = clamp((max(dot(-lightdir, lightforward[i]), .0) - cos(lightfov[i] * .5 * 3.1415 / 180.0)) * 5.0, .0, 1.0);
			vec3 light = lightcol[i] / max(dot(lightpos[i] - worldpos, lightpos[i] - worldpos), light_min_dist);
			fColor.rgb += lightang * light * evaluatebrdf(1, cosa_light, cosa_mirrorlight, lightdir, indir, diffusecol, specularcol, exp);
		}
	}
	fColor.rgb += emission_padding.rgb + ambient_light * diffusecol;
	fColor.a = alpha;
	fColor.rgb *= alpha;
	//fColor.rgb = textureLod(shadowmap, vec3(gl_FragCoord.xy / 800.0, .0), 0).rgb;
	//fColor.rgb = n*.5 + vec3(.5);
	//fColor.rgb = vec3(exp) * diffusecol;
}