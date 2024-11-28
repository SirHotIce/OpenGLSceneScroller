#version 400

uniform mat4 PVM; // Projection-View-Model matrix
uniform float time; // Time for animation
uniform float wave_strength; // Strength of wave animation

in vec3 pos_attrib; // Position of mesh vertices
in vec2 tex_coord_attrib; // Texture coordinates
in vec3 normal_attrib; // Vertex normal

out vec2 tex_coord;

void main(void)
{
    // Calculate wave effect using a sine function
    float wave = sin(pos_attrib.x * 5.0 + time) * wave_strength;

    // Offset the vertex position by the wave effect
    vec3 animated_position = pos_attrib + vec3(0.0, wave, 0.0);

    // Transform the animated position
    gl_Position = PVM * vec4(animated_position, 1.0);

    // Pass texture coordinates to fragment shader
    tex_coord = tex_coord_attrib;
}
