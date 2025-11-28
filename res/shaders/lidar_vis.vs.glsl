#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;

out vec3 worldPos;
out float vertexObstacleDepth;
out vec3 vertexNormal;
out vec2 vertexTexCoord;
flat out int vertexIsInFOV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lidarPosition;
uniform float maxDistance;
uniform samplerCube depthCubemap;
uniform float vFov;
uniform float hFov;
uniform vec3 lidarDirection;

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
    vec3 dir = normalize(aPos);

    float obstacleDepth = texture(depthCubemap, dir).r;
    float obstacleDistance = obstacleDepth * maxDistance;

    // 1. check fov range
    vertexIsInFOV = int(isInFOV(dir));
    float isInFOV = step(0.1, float(vertexIsInFOV));

    // 2. check if on envelope surface (occluded by obstacle)
    float notObstacle = step(maxDistance - 0.01, obstacleDistance);

    // 3. calculate world position
    worldPos = lidarPosition + dir * maxDistance * notObstacle * isInFOV;
    vertexObstacleDepth = obstacleDepth;
    vertexNormal = aNormal;
    vertexTexCoord = aTexCoord * isInFOV;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}