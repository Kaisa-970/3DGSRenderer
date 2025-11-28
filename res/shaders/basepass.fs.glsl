#version 410 core

in vec3 worldPos;
in vec3 worldNormal;
in vec3 vColor;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gColor;
layout(location = 3) out float gDepth;

uniform vec3 uColor;

void main() {
    gPosition = worldPos;
    gNormal = normalize(worldNormal);
    gColor = vColor * uColor;
    //gDepth = gl_FragCoord.z;
}