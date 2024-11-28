#version 400

uniform sampler2D diffuse_tex;
uniform vec4 tintColor;
uniform bool useTexture; // Boolean to indicate if a texture is being used

in vec2 tex_coord;

out vec4 fragcolor;

void main(void) {
    vec4 texColor = useTexture ? texture(diffuse_tex, tex_coord) : vec4(1.0); // Use white if no texture
    fragcolor = texColor * tintColor;
}
