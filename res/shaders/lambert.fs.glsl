#version 330 core
out vec4 FragColor;

// 从顶点着色器传入
in vec3 FragPos;     // 世界空间中的片段位置
in vec3 Normal;      // 世界空间中的法线

// 光照参数
uniform vec3 lightPos;       // 光源位置
uniform vec3 lightColor;     // 光源颜色
uniform vec3 objectColor;    // 物体颜色
uniform vec3 viewPos;        // 相机位置

// 环境光、漫反射、镜面反射强度
uniform float ambientStrength;   // 环境光强度 (默认 0.1)
uniform float specularStrength;  // 镜面反射强度 (默认 0.5)
uniform int shininess;           // 镜面反射指数 (默认 32)

void main()
{
    // 1. 环境光 (Ambient)
    vec3 ambient = ambientStrength * lightColor;
    
    // 2. 漫反射 (Diffuse) - Lambert 光照
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);  // Lambert 余弦定律
    vec3 diffuse = diff * lightColor;
    
    // 3. 镜面反射 (Specular) - Phong 模型
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 组合所有光照分量
    vec3 result = (ambient + diffuse + specular) * objectColor;
    
    FragColor = vec4(result, 1.0);
}

