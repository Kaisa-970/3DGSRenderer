#version 430 core

in vec3 worldPos;

out vec4 fragColor;

uniform vec3 lidarPosition;
uniform vec3 lidarDirection;
uniform float fov;
uniform float maxDistance;
uniform vec3 color;
uniform samplerCube depthCubemap;
uniform sampler2D sceneDepthTexture;   // 相机视角的场景深度
uniform vec2 screenSize;

void main() {
    // 计算当前片段相对于雷达的方向和距离
    vec3 toFragment = worldPos - lidarPosition;
    float pointDistance = length(toFragment);
    vec3 dir = normalize(toFragment);
    
    // 1. 检查FOV范围
    float cosAngle = dot(dir, lidarDirection);
    float halfFovCos = cos(fov * 0.5);
    // if (cosAngle < halfFovCos) {
    //     discard;  // 不在FOV范围内
    // }
    
    // 2. 从深度Cubemap中采样障碍物距离
    float obstacleDepth = texture(depthCubemap, dir).r;
    float obstacleDistance = obstacleDepth * maxDistance;  // 反归一化
    
    // 3. 关键比较：球面点距离 vs 障碍物距离
    // 如果球面点更远，说明这个方向被障碍物遮挡了
    // if (pointDistance > obstacleDistance + 0.01) {  // 小偏移避免z-fighting
    //     discard;  // 被遮挡
    // }

    float alpha = 0.4;
    vec3 finalColor = color;
    
    if (pointDistance > obstacleDistance + 0.05) {
        // 被遮挡的区域：只在障碍物距离附近绘制"封口面"
        // 通过距离衰减来形成封口效果
        float distFromObstacle = pointDistance - obstacleDistance;
        float fadeRange = 0.5;  // 封口面的厚度
        
        if (distFromObstacle > fadeRange) {
            discard;  // 距离障碍物太远，不绘制
        }
        
        // 在封口面上使用不同的颜色（稍暗）
        float fadeFactor = 1.0 - (distFromObstacle / fadeRange);
        alpha = 0.6 * fadeFactor;
        finalColor = color * 0.7;  // 封口面稍暗
    }
    
    // // 4. 渐变效果
    // float distanceFade = 1.0 - (pointDistance / maxDistance);
    // float edgeFade = (cosAngle - halfFovCos) / (1.0 - halfFovCos);
    // edgeFade = clamp(edgeFade, 0.0, 1.0);
    
    // float alpha = color.a * distanceFade * sqrt(edgeFade);

    vec2 screenUV = gl_FragCoord.xy / screenSize;
    float sceneDepth = texture(sceneDepthTexture, screenUV).r;
    if (sceneDepth < gl_FragCoord.z - 0.0001) {
        discard;
    }
    
    fragColor = vec4(finalColor, alpha);
    //fragColor = vec4(vec3(obstacleDepth), 1.0);
}