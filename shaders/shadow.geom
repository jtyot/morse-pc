#version 330
 
layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

uniform int layer;

void main(void){
	gl_Layer = layer;
	for(int i=0; i<gl_in.length(); i++){
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}