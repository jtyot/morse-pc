#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in float lifetime;

uniform mat4 uModelToWorld;
uniform mat4 uWorldToClip;

smooth out vec3 acol;
smooth out vec3 worldpos;

vec3 temptorgb(float T) {
	vec3 ret = vec3(.0);
	
    	if(T<= 66.0) {
    		ret.rg = vec2( 255.0, 99.4708025861 * log(T) - 161.1195681661);
		if(T<=19.0)
			ret.b = .0;
		else
			ret.b = 138.5177312231 * log(T-10.0) - 305.0447927307;
   	} else
        	ret = vec3( 329.698727446  * pow(T-60.0, -0.1332047592), 288.1221695283 * pow(T-60.0, -0.0755148492), 255.0);
    	
	return vec3(clamp(ret/255.0, vec3(.0), vec3(1.0)));
}

void main()
{
	vec4 pos = uModelToWorld * vec4(aPosition, 1.0);
	gl_Position = uWorldToClip * pos;
	float t1 = lifetime;//max(lifetime - .5, .0) * 2.0;
	vec3 emission = temptorgb(t1 * t1 * 40.0) * (pow(t1, 2.0)*.9 + .1) * 200.0;
	acol.rgb = emission;
	worldpos = pos.xyz;
}