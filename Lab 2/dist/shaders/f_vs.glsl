#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 PVM;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 fragPosition;
out vec3 fragNormal;

void main() {
    gl_Position = PVM * vec4(position, 1.0);
    fragPosition = vec3(model * vec4(position, 1.0));
    fragNormal = normalize(normalMatrix * normal);
}
