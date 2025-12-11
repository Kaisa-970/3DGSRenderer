#version 430 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uColor = vec3(0.2, 0.8, 1.0);
uniform float u_time = 0.0;
uniform float u_alpha = 0.6;
uniform vec3 u_viewPos;

void main()
{
    // // 简单的呼吸/菲涅尔混合效果
    // vec3 n = normalize(vNormal);
    // vec3 viewDir = normalize(u_viewPos - vWorldPos);
    // float viewFacing = dot(n, viewDir);
    // float pulse = 1.0;//0.5 + 0.5 * sin(u_time * 3.0 + vWorldPos.y * 2.0);
    // float rim = pow(1.0 - viewFacing, 5.0);
    // if(rim < 0.2) discard;
    // float glow = mix(0.3, 1.0, rim) * pulse;

    // vec3 color = uColor * glow;
    // float alpha = clamp(u_alpha * (0.4 + 0.6 * rim), 0.0, 1.0);
    // FragColor = vec4(color, alpha);

    float us = 0.01;
    if(vTexCoord.x > us && vTexCoord.x < 1.0 - us && vTexCoord.y > us && vTexCoord.y < 1.0 - us)
    {
        //FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        discard;
    }
    else
    {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
}

