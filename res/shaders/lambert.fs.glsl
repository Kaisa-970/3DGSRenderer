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
uniform sampler2D u_shadowTexture;

uniform int numLights;

uniform mat4 lightSpaceMat;
uniform vec3 directionalLightDirection;

float CalculateShadow(vec2 texCoord, float currentDepth, float bias)
{
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_shadowTexture, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_shadowTexture, texCoord + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}
void main()
{
    vec3 FragPos = texture(u_positionTexture, texCoord).rgb;
    vec3 Normal = texture(u_normalTexture, texCoord).rgb;
    if(length(Normal) < 0.01) discard;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm = normalize(Normal);

    vec3 diffuseColor = texture(u_diffuseTexture, texCoord).rgb;
    vec3 specularColor = texture(u_specularTexture, texCoord).rgb;
    vec4 shadowCoord = lightSpaceMat * vec4(FragPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xyz = shadowCoord.xyz * 0.5 + 0.5;
    float shadow = 0.0;
    if (shadowCoord.z > 0.0 && shadowCoord.z < 1.0 &&
    shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 &&
    shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0) {
        //float closestDepth = texture(u_shadowTexture, shadowCoord.xy).r;
        float currentDepth = shadowCoord.z;
        float bias = max(0.02 * (1.0 - dot(norm, -directionalLightDirection)), 0.002);

        shadow = CalculateShadow(shadowCoord.xy, currentDepth, bias);//currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }

    vec3 baseColor = diffuseColor;

    // 1. 环境光 (Ambient)
    vec3 ambient = ambientStrength * baseColor;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    for(int i = 0; i < numLights; i++){
        // 2. 漫反射 (Diffuse)
        vec3 lightDir;
        float attenuation = 1.0;
        if(i == 0)
        {
            lightDir = normalize(-directionalLightDirection);
        }
        else
        {
            lightDir = normalize(lights[i].position - FragPos);
            float dist = length(lights[i].position - FragPos);
            attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        }
        float diff = max(dot(norm, lightDir), 0.0);  // Lambert 余弦定律
        diffuse += diff * lights[i].color * diffuseStrength * attenuation;

        // 3. 镜面反射 (Specular) - Blinn-Phong 模型
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
        specular += spec * lights[i].color * specularColor * specularStrength * attenuation;
    }

    // 组合所有光照分量
    vec3 result = ambient * baseColor + (diffuse * baseColor + specular) * (1.0 - shadow);

    FragColor = vec4(result, 1.0);
}

