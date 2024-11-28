#version 400
uniform sampler2D diffuse_tex;
uniform float time;
uniform float brightness;

in vec2 tex_coord; 

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	fragcolor = texture(diffuse_tex, tex_coord)*brightness; //read color from texture and assign to outgoing fragment color
}




















