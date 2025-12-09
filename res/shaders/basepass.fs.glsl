#version 430 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gDiffuse;
layout(location = 3) out vec3 gSpecular;
layout(location = 4) out float gShininess;
layout(location = 5) out float gDepth;

uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;
uniform float u_shininess;
uniform vec3 u_diffuseColor;
uniform vec3 u_specularColor;
uniform vec3 uColor;

void main() {
    gPosition = worldPos;
    gNormal = normalize(worldNormal);
    vec3 diffuseColor = texture(u_diffuseTexture, texCoord).rgb;
    gDiffuse = u_diffuseColor * diffuseColor;
    gSpecular = u_specularColor * texture(u_specularTexture, texCoord).rgb;
    gShininess = u_shininess;
    //gDepth = gl_FragCoord.z;
}