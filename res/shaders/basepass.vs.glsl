#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;

out vec3 worldPos;
out vec3 worldNormal;
out vec3 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    worldPos = vec3(model * vec4(aPos, 1.0));
    worldNormal = mat3(transpose(inverse(model))) * aNormal;
    vColor = aColor;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}