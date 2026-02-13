#version 430 core

in vec2 texCoord;
out vec4 FragColor;

const int MAX_LIGHTS = 32;
struct Light{
    vec3 position;
    vec3 color;
    float intensity;
};

// 光照参数
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;        // 相机位置

// 环境光、漫反射、镜面反射强度
uniform float ambientStrength;   // 环境光强度 (默认 0.1)
uniform float diffuseStrength;   // 漫反射强度 (默认 0.9)
uniform float specularStrength;  // 镜面反射强度 (默认 0.5)
uniform float shininess;           // 镜面反射指数 (默认 32)

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;

uniform int numLights;

void main()
{
    vec3 FragPos = texture(u_positionTexture, texCoord).rgb;
    vec3 Normal = texture(u_normalTexture, texCoord).rgb;
    if(length(Normal) < 0.01) discard;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm = normalize(Normal);

    vec3 diffuseColor = texture(u_diffuseTexture, texCoord).rgb;
    vec3 specularColor = texture(u_specularTexture, texCoord).rgb;

    vec3 baseColor = diffuseColor;

    // 1. 环境光 (Ambient)
    vec3 ambient = ambientStrength * baseColor;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    for(int i = 0; i < numLights; i++){
        // 2. 漫反射 (Diffuse)
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float dist = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        float diff = max(dot(norm, lightDir), 0.0);  // Lambert 余弦定律
        diffuse += diff * lights[i].color * diffuseStrength * attenuation;

        // 3. 镜面反射 (Specular) - Blinn-Phong 模型
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
        specular += spec * lights[i].color * specularColor * specularStrength * attenuation;
    }

    // 组合所有光照分量
    vec3 result = (ambient + diffuse) * baseColor + specular;

    FragColor = vec4(result, 1.0);
}

