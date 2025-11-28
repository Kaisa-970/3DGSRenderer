#version 410 core

in vec2 texCoord;
out vec4 FragColor;

// 光照参数
uniform vec3 lightPos;       // 光源位置
uniform vec3 lightColor;     // 光源颜色
uniform vec3 objectColor;    // 物体颜色
uniform vec3 viewPos;        // 相机位置

// 环境光、漫反射、镜面反射强度
uniform float ambientStrength;   // 环境光强度 (默认 0.1)
uniform float specularStrength;  // 镜面反射强度 (默认 0.5)
uniform int shininess;           // 镜面反射指数 (默认 32)

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_colorTexture;

void main()
{
    vec3 FragPos = texture(u_positionTexture, texCoord).rgb;
    vec3 Normal = texture(u_normalTexture, texCoord).rgb;
    vec3 objectColor = texture(u_colorTexture, texCoord).rgb;

    if (length(Normal) < 0.001)
    {
        FragColor = vec4(0.2f, 0.3f, 0.3f, 1.0);
        return;
    }

    // 1. 环境光 (Ambient)
    vec3 ambient = ambientStrength * lightColor;
    
    // 2. 漫反射 (Diffuse) - Lambert 光照
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);  // Lambert 余弦定律
    vec3 diffuse = diff * lightColor;
    
    // 3. 镜面反射 (Specular) - Blinn-Phong 模型
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 组合所有光照分量
    vec3 result = (ambient + diffuse + specular) * objectColor;
    
    FragColor = vec4(result, 1.0);
    //FragColor = vec4(norm, 1.0);
}

