#version 410 core

in vec3 fragPos;

uniform vec3 lightPos;   // 激光雷达位置
uniform float farPlane;  // 最大距离

void main() {
    // 计算片段到激光雷达的距离，并归一化到[0,1]
    float distance = length(fragPos - lightPos);
    gl_FragDepth = distance / farPlane;
}