// SpherePrimitive 和 QuadPrimitive 使用示例

#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"

// 示例 1: 创建球体
void example_sphere() {
    // 创建一个白色球体，半径 1.0，36 经线，18 纬线
    Renderer::SpherePrimitive sphere1(1.0f, 36, 18, false);
    
    // 创建一个彩色球体（渐变色），半径 2.0，细节更高
    Renderer::SpherePrimitive sphere2(2.0f, 64, 32, true);
    
    // 使用 Lambert 着色器绘制
    Renderer::Shader lambertShader = Renderer::Shader::fromFiles(
        "res/shaders/lambert.vs.glsl", 
        "res/shaders/lambert.fs.glsl"
    );
    
    lambertShader.use();
    
    // 设置变换矩阵
    lambertShader.setMat4("model", modelMatrix);
    lambertShader.setMat4("view", viewMatrix);
    lambertShader.setMat4("projection", projectionMatrix);
    
    // 设置光照参数
    lambertShader.setVec3("lightPos", 2.0f, 2.0f, 2.0f);
    lambertShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    lambertShader.setVec3("objectColor", 0.8f, 0.3f, 0.3f);  // 红色球体
    lambertShader.setVec3("viewPos", camera.getPosition());
    lambertShader.setFloat("ambientStrength", 0.2f);
    lambertShader.setFloat("specularStrength", 0.5f);
    lambertShader.setInt("shininess", 32);
    
    // 绘制球体
    sphere1.draw(lambertShader);
}

// 示例 2: 创建四边形
void example_quad() {
    // 创建一个白色四边形，尺寸 2.0
    Renderer::QuadPrimitive quad1(2.0f, false);
    
    // 创建一个彩色四边形（四个顶点不同颜色）
    Renderer::QuadPrimitive quad2(1.5f, true);
    
    // 使用简单着色器绘制
    Renderer::Shader shader = Renderer::Shader::fromFiles(
        "res/shaders/cube.vs.glsl", 
        "res/shaders/cube.fs.glsl"
    );
    
    shader.use();
    shader.setMat4("model", modelMatrix);
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    
    // 绘制四边形
    quad1.draw(shader);
}

// 示例 3: 组合使用 - 太阳系场景
void example_solar_system() {
    // 太阳（大球体，彩色）
    Renderer::SpherePrimitive sun(2.0f, 36, 18, true);
    
    // 地球（中等球体）
    Renderer::SpherePrimitive earth(0.8f, 32, 16, false);
    
    // 月球（小球体）
    Renderer::SpherePrimitive moon(0.3f, 24, 12, false);
    
    // 轨道平面（大四边形，作为参考平面）
    Renderer::QuadPrimitive orbitPlane(10.0f, false);
    
    Renderer::Shader lambertShader = Renderer::Shader::fromFiles(
        "res/shaders/lambert.vs.glsl", 
        "res/shaders/lambert.fs.glsl"
    );
    
    lambertShader.use();
    
    // 绘制太阳（中心位置）
    Renderer::Matrix4 sunModel = Renderer::Matrix4::identity();
    lambertShader.setMat4("model", sunModel.m);
    lambertShader.setVec3("objectColor", 1.0f, 0.8f, 0.0f);  // 黄色
    sun.draw(lambertShader);
    
    // 绘制地球（围绕太阳）
    Renderer::Matrix4 earthModel = Renderer::Matrix4::translation(5.0f, 0.0f, 0.0f);
    lambertShader.setMat4("model", earthModel.m);
    lambertShader.setVec3("objectColor", 0.2f, 0.5f, 0.8f);  // 蓝色
    earth.draw(lambertShader);
    
    // 绘制月球（围绕地球）
    Renderer::Matrix4 moonModel = Renderer::Matrix4::translation(6.5f, 0.0f, 0.0f);
    lambertShader.setMat4("model", moonModel.m);
    lambertShader.setVec3("objectColor", 0.7f, 0.7f, 0.7f);  // 灰色
    moon.draw(lambertShader);
}

