#version 410 core

in vec3 worldPos;
in float vertexObstacleDepth;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
flat in int vertexIsInFOV;

out vec4 fragColor;

uniform vec3 lidarPosition;
uniform vec3 lidarDirection;
uniform float vFov;
uniform float hFov;
uniform float maxDistance;
uniform vec3 color;
uniform samplerCube depthCubemap;
uniform sampler2D sceneDepthTexture;   // 相机视角的场景深度
uniform vec2 screenSize;

bool isInFOV(vec3 dir) {
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 forward = normalize(lidarDirection);
    if (abs(dot(up, forward)) > 0.99) {
        up = vec3(0.0, 0.0, 1.0);
    }

    vec3 right = normalize(cross(forward, up));
    up = normalize(cross(right, forward));

    float forwardDot = dot(forward, dir);
    float rightDot = dot(right, dir);
    float upDot = dot(up, dir);

    float vAngle = atan(upDot, forwardDot);
    float hAngle = atan(rightDot, forwardDot);
    return (abs(vAngle) < vFov * 0.5) && (abs(hAngle) < hFov * 0.5);
}

void main() {
    // 计算当前片段相对于雷达的方向和距离
    vec3 toFragment = worldPos - lidarPosition;
    float pointDistance = length(toFragment);
    if (pointDistance < 0.001)
    {
        fragColor = vec4(color, 0.5);
        return;
    }
    vec3 dir = normalize(toFragment);
    
    // 1. 检查FOV范围
    // float cosAngle = dot(dir, lidarDirection);
    // float halfFovCos = cos(fov * 0.5);
    // if (cosAngle < halfFovCos) {
    //     discard;  // 不在FOV范围内
    // }
    if (vertexIsInFOV == 0) {
        discard;  // 不在FOV范围内
    }
    
    // 2. 相机视角遮挡检测
    vec2 screenUV = gl_FragCoord.xy / screenSize;
    float sceneDepth = texture(sceneDepthTexture, screenUV).r;
    if (sceneDepth < gl_FragCoord.z - 0.0001) {
        discard;
    }
    
    // 3. 判断是否在包络面上（被障碍物遮挡的区域）
    bool isOnEnvelope = (pointDistance < maxDistance - 0.01);
    
    // 4. 渐变效果
    float distanceFade = 1.0 - (pointDistance / maxDistance);
    // float edgeFade = (cosAngle - halfFovCos) / (1.0 - halfFovCos);
    // edgeFade = clamp(edgeFade, 0.0, 1.0);
    
    // float alpha = distanceFade * sqrt(edgeFade) * 0.5;
    vec3 finalColor = color;
    
    // 包络面用不同的颜色/透明度
    // if (isOnEnvelope) {
    //     finalColor = color * 0.8;  // 稍暗
    //     alpha *= 1.2;  // 更明显
    // }

    // distanceFade = clamp(distanceFade, 0.0, 1.0);
    // distanceFade = mix(distanceFade, 0.9, 0.8);
    fragColor = vec4(finalColor, 0.5);
    // fragColor = vec4(vertexNormal, 0.5);
    // fragColor = vec4(vertexTexCoord, 0.0, 0.5);
}