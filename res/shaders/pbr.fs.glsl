#version 430 core

in vec2 texCoord;
out vec4 FragColor;

const float PI = 3.14159265359;
const int MAX_LIGHTS = 32;

struct Light
{
    vec3 position;
    vec3 color;
    float intensity;
};

uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;
uniform int numLights;

uniform mat4 lightSpaceMat;
uniform vec3 directionalLightDirection;

uniform float ambientStrength;

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;       // PBR: albedo
uniform sampler2D u_metallicRoughnessTexture;
uniform sampler2D u_shadowTexture;
uniform sampler2D u_ssaoTexture;
uniform int u_useSSAO;
uniform float u_ssaoStrength;

// ---- 阴影 ----
float CalculateShadow(vec2 st, float currentDepth, float bias)
{
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_shadowTexture, 0);
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            shadow += (currentDepth - bias > texture(u_shadowTexture, st + vec2(x, y) * texelSize).r) ? 1.0 : 0.0;
    return shadow / 9.0;
}

// ---- PBR: GGX / Trowbridge-Reitz ----
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return (denom > 0.0) ? (a2 / denom) : 0.0;
}

// ---- PBR: Schlick-GGX 几何 ----
float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

// ---- PBR: Fresnel-Schlick ----
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 FragPos = texture(u_positionTexture, texCoord).rgb;
    vec3 N = normalize(texture(u_normalTexture, texCoord).rgb);
    if (length(N) < 0.01) discard;

    vec3 albedo = texture(u_diffuseTexture, texCoord).rgb;
    vec2 mr = texture(u_metallicRoughnessTexture, texCoord).rg;
    float metallic = mr.r;
    float roughness = max(mr.g, 0.04);

    vec3 V = normalize(viewPos - FragPos);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float ao = (u_useSSAO != 0) ? texture(u_ssaoTexture, texCoord).r : 1.0;
    ao = (u_useSSAO != 0) ? mix(1.0, ao, u_ssaoStrength) : 1.0;
    vec3 ambient = ambientStrength * albedo * ao;

    float shadow = 0.0;
    vec4 shadowCoord = lightSpaceMat * vec4(FragPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xyz = shadowCoord.xyz * 0.5 + 0.5;
    if (shadowCoord.z > 0.0 && shadowCoord.z < 1.0 &&
        shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 &&
        shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
    {
        float bias = max(0.02 * (1.0 - dot(N, -directionalLightDirection)), 0.002);
        shadow = CalculateShadow(shadowCoord.xy, shadowCoord.z, bias);
    }

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < numLights; i++)
    {
        vec3 L_dir;
        vec3 radiance;
        if (i == 0)
        {
            L_dir = normalize(-directionalLightDirection);
            radiance = lights[i].color * lights[i].intensity;
        }
        else
        {
            vec3 toLight = lights[i].position - FragPos;
            L_dir = normalize(toLight);
            float dist = length(toLight);
            float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
            radiance = lights[i].color * lights[i].intensity * atten;
        }

        float NdotL = max(dot(N, L_dir), 0.0);
        if (NdotL <= 0.0) continue;

        vec3 H = normalize(V + L_dir);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L_dir, roughness);
        float HdotV = max(dot(H, V), 0.0);
        vec3 F = fresnelSchlick(HdotV, F0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        // 分母过小会在掠射角（边缘）导致镜面爆炸，出现白色光晕；用较大下限稳定
        float denom = max(4.0 * NdotV * NdotL, 0.02);
        vec3 specular = (NDF * G * F) / denom;
        // 限制单次镜面贡献，避免 HDR 下边缘过亮
        specular = min(specular, vec3(2.0));
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 color = ambient + Lo * (1.0 - shadow);
    FragColor = vec4(color, 1.0);
}
