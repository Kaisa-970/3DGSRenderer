#include <cmath>
#include "Renderer/Window.h"
#include "Renderer/RendererContext.h"
#include "Renderer/Camera.h"
#include "Logger/Log.h"
#include "Renderer/Primitives/CubePrimitive.h"
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Primitives/QuadPrimitive.h"
#include "Renderer/FinalPass.h"
#include "Renderer/GeometryPass.h"
#include "Renderer/LightingPass.h"
#include "Renderer/MathUtils/Random.h"
#include "Renderer/PostProcessPass.h"
#include "Renderer/LidarSensor.h"

#ifdef GSRENDERER_OS_WINDOWS
#include <windows.h>
#endif

// 鼠标状态
struct MouseState {
    bool firstMouse = true;
    bool isRotating = false;
    float lastX = 0.0f;  // 初始值无关紧要，firstMouse 会处理第一次输入
    float lastY = 0.0f;
} mouseState;

static constexpr float PI = 3.1415926f;
static constexpr float D2R_FACTOR = PI / 180.0f;
static constexpr float R2D_FACTOR = 180.0f / PI;
#define DEG2RAD(deg) (deg * D2R_FACTOR)
#define RAD2DEG(rad) (rad * R2D_FACTOR)

const int WIN_WIDTH = 1600;
const int WIN_HEIGHT = 900;

int main(int argc, char* argv[]) {
#ifdef GSRENDERER_OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 初始化日志系统
    Logger::Log::Init();
    
    LOG_INFO("Start 3DGS Renderer...");
    
    try {
        // 初始化 GLFW
        if (!Renderer::Window::initGLFW()) {
            LOG_ERROR("Failed to initialize GLFW");
            return -1;
        }
        LOG_INFO("Successfully initialized GLFW");

        // 创建窗口（自动初始化 OpenGL 上下文）
        Renderer::Window window(WIN_WIDTH, WIN_HEIGHT, "3DGS Renderer");
        LOG_INFO("Successfully created window: {}x{}", WIN_WIDTH, WIN_HEIGHT);
        
        // 创建渲染器上下文
        Renderer::RendererContext rendererContext;
        LOG_INFO("Successfully created renderer context");
        
        // 创建相机（根据模型自动计算位置）
        Renderer::Vector3 camPos(0.0f, 2.0f, 5.0f);
        Renderer::Camera camera(
            camPos,  // 位置（模型中心前方）
            Renderer::Vector3(0.0f, 1.0f, 0.0f),        // 世界上方向
            -90.0f,  // yaw: -90度 朝向 -Z 方向（看向模型）
            -20.0f     // pitch: 0度 水平视角
        );

        camera.setMovementSpeed(3.0f);  // 增大移动速度，因为场景较大
        camera.setMouseSensitivity(0.1f);
        
        float x, y, z;
        camera.getPosition(x, y, z);
        LOG_INFO("Successfully created camera at position: ({}, {}, {})", x, y, z);

        // 设置鼠标移动回调
        // 注意：在 GLFW_CURSOR_DISABLED 模式下，xpos/ypos 是虚拟坐标，会持续累加
        window.setMouseMoveCallback([&camera, &window](double xpos, double ypos) {
            if (!window.isMouseButtonPressed(Renderer::MouseButton::Right)) {
                mouseState.firstMouse = true;  // 重置状态
                return;
            }

            if (mouseState.firstMouse) {
                mouseState.lastX = static_cast<float>(xpos);
                mouseState.lastY = static_cast<float>(ypos);
                mouseState.firstMouse = false;
                return;  // 跳过第一次输入，避免初始跳跃
            }

            float xoffset = static_cast<float>(xpos) - mouseState.lastX;
            float yoffset = mouseState.lastY - static_cast<float>(ypos);
            
            // 防止窗口焦点切换时产生的巨大跳跃（阈值：100像素）
            if (std::abs(xoffset) > 100.0f || std::abs(yoffset) > 100.0f) {
                mouseState.lastX = static_cast<float>(xpos);
                mouseState.lastY = static_cast<float>(ypos);
                return;  // 忽略异常大的偏移
            }
            
            mouseState.lastX = static_cast<float>(xpos);
            mouseState.lastY = static_cast<float>(ypos);

            camera.processMouseMovement(xoffset, yoffset);
        });

        // 设置鼠标滚轮回调
        window.setMouseScrollCallback([&camera](double xoffset, double yoffset) {
            camera.processMouseScroll(static_cast<float>(yoffset));
        });

        // 先设置光标模式为禁用（FPS模式），这会锁定并隐藏鼠标
        //window.setCursorMode(Renderer::Window::CursorMode::Disabled);

        // create primitives
        Renderer::CubePrimitive cubePrimitive(1.0f, false);
        Renderer::SpherePrimitive spherePrimitive(1.0f, 64, 32, false);
        Renderer::QuadPrimitive quadPrimitive(10.0f, false);

        // 测试相机矩阵
        float viewMatrix[16];
        float projMatrix[16];
        camera.getViewMatrix(viewMatrix);
        camera.getPerspectiveMatrix(projMatrix, 50.0f, static_cast<float>(WIN_WIDTH) / static_cast<float>(WIN_HEIGHT), 0.01f, 1000.0f);

        // create passes
        Renderer::GeometryPass geometryPass;
        Renderer::LightingPass lightingPass;
        Renderer::PostProcessPass postProcessPass;
        Renderer::FinalPass finalPass;

        std::vector<Renderer::Matrix4> sphereModels;
        std::vector<Renderer::Vector3> sphereColors;
        float minX = -10.0f;
        float maxX = 10.0f;
        float minY = -10.0f;
        float maxY = 10.0f;
        float minZ = -10.0f;
        float maxZ = 10.0f;
        float minRadius = 0.1f;
        float maxRadius = 1.0f;
        for (int i = 0; i < 30; i++)
        {
            Renderer::Matrix4 sphereModel = Renderer::Matrix4::identity();
            float radius = Renderer::Random::randomFloat(minRadius, maxRadius);
            sphereModel.scaleBy(radius, radius, radius);
            sphereModel.translate(Renderer::Random::randomFloat(minX, maxX), Renderer::Random::randomFloat(minY, maxY), Renderer::Random::randomFloat(minZ, maxZ));
            sphereModel = sphereModel.transpose();
            sphereModels.push_back(sphereModel);

            sphereColors.push_back(Renderer::Random::randomColor());
        }

        Renderer::Vector3 sphereColor = Renderer::Random::randomColor();

        Renderer::LidarSensor lidarSensor(
            Renderer::Vector3(0.0f, 0.0f, 0.0f), 
            Renderer::Vector3(1.0f, 0.0f, 0.0f), 
            DEG2RAD(90.0f), 
            DEG2RAD(150.0f),
            10.0f);

        Renderer::Matrix4 sphereModel = Renderer::Matrix4::identity();
        sphereModel.scaleBy(0.1f, 0.1f, 0.1f);
        sphereModel.translate(0.0f, 0.0f, 0.0f);
        sphereModel = sphereModel.transpose();

        float lastTime = 0.0f;

        // render loop
        while (!window.shouldClose()) {
            float currentTime = static_cast<float>(Renderer::Window::getTime());
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            {
                // 处理键盘输入
                if (window.isKeyPressed(Renderer::Key::Escape))
                    break;
                
                if (window.isKeyPressed(Renderer::Key::W))
                    camera.processKeyboard(Renderer::CameraMovement::Forward, deltaTime);
                if (window.isKeyPressed(Renderer::Key::S))
                    camera.processKeyboard(Renderer::CameraMovement::Backward, deltaTime);
                if (window.isKeyPressed(Renderer::Key::A))
                    camera.processKeyboard(Renderer::CameraMovement::Left, deltaTime);
                if (window.isKeyPressed(Renderer::Key::D))
                    camera.processKeyboard(Renderer::CameraMovement::Right, deltaTime);
                if (window.isKeyPressed(Renderer::Key::E))
                    camera.processKeyboard(Renderer::CameraMovement::Up, deltaTime);
                if (window.isKeyPressed(Renderer::Key::Q))
                    camera.processKeyboard(Renderer::CameraMovement::Down, deltaTime);
            
            }

            Renderer::Vector3 lightPos(0.0f, 3.0f, 3.0f);
            float curX = 3.0f * std::sin(currentTime);
            float curZ = 3.0f * std::cos(currentTime);
            lightPos.x = curX;
            lightPos.z = curZ;

            // update view matrix
            camera.getViewMatrix(viewMatrix);
            
            rendererContext.clear(0.0f, 0.0f, 0.0f, 1.0f);
            
            Renderer::Vector3 lidarDirection(1.0f, 0.0f, 0.0f);
            lidarDirection = lightPos.normalized();
            //lidarSensor.setDirection(lidarDirection);
            //lidarSensor.setPosition(lightPos);
            //lidarSensor.setPosition(Renderer::Vector3(0.0f, 5.0f, 0.0f));

            // update model matrices
            Renderer::Matrix4 sphereModel2 = Renderer::Matrix4::identity();
            float centX = 3.0f;
            float posX = centX + std::sin(currentTime);
            sphereModel2.translate(posX, 0.0f, 0.0f);
            sphereModel2 = sphereModel2.transpose();

            Renderer::Matrix4 quadModel = Renderer::Matrix4::identity();
            quadModel.scaleBy(10.0f, 10.0f, 10.0f);
            quadModel.rotate(DEG2RAD(-90.0f), Renderer::Vector3(1.0f, 0.0f, 0.0f));
            quadModel.translate(0.0f, -0.01f, 0.0f);
            quadModel = quadModel.transpose();

            Renderer::Matrix4 quadModel2 = Renderer::Matrix4::identity();
            quadModel2.scaleBy(0.2f, 0.2f, 0.2f);
            quadModel2.rotate(DEG2RAD(30.0f), Renderer::Vector3(0.0f, 1.0f, 0.0f));
            quadModel2.translate(0.0f, 0.0f, -0.1f);
            quadModel2 = quadModel2.transpose();

            Renderer::Matrix4 quadModel3 = Renderer::Matrix4::identity();
            quadModel3.scaleBy(0.2f, 0.2f, 0.2f);
            quadModel3.rotate(DEG2RAD(-30.0f), Renderer::Vector3(0.0f, 1.0f, 0.0f));
            quadModel3.translate(0.0f, 0.0f, -0.1f);
            quadModel3 = quadModel3.transpose();

            // render lidar depth cubemap
            {
                std::vector<std::pair<Renderer::Primitive*, Renderer::Matrix4>> primitives;

                primitives.push_back(std::make_pair(&quadPrimitive, quadModel));
                //primitives.push_back(std::make_pair(&spherePrimitive, sphereModel2));
                //primitives.push_back(std::make_pair(&quadPrimitive, quadModel2));
                //primitives.push_back(std::make_pair(&quadPrimitive, quadModel3));
                
                for (int i = 0; i < sphereModels.size(); i++)
                {
                    primitives.push_back(std::make_pair(&spherePrimitive, sphereModels[i]));
                }
    
                lidarSensor.renderDepth(primitives);
            }

            rendererContext.viewPort(0, 0, WIN_WIDTH, WIN_HEIGHT);

            // geometry pass
            {
                geometryPass.begin(viewMatrix, projMatrix);

                for (int i = 0; i < sphereModels.size(); i++)
                {
                    geometryPass.render(&spherePrimitive, sphereModels[i].m, sphereColors[i]);
                }
                //geometryPass.render(&spherePrimitive, sphereModel2.m, sphereColor);
                geometryPass.render(&spherePrimitive, sphereModel.m, Renderer::Vector3(1.0f, 0.0f, 0.0f));
                geometryPass.render(&quadPrimitive, quadModel.m, Renderer::Vector3(0.5f, 0.5f, 0.5f));
                //geometryPass.render(&quadPrimitive, quadModel2.m, Renderer::Vector3(0.5f, 0.5f, 0.5f));
                //geometryPass.render(&quadPrimitive, quadModel3.m, Renderer::Vector3(0.5f, 0.5f, 0.5f));
                geometryPass.end();
            }

            // lighting pass
            lightingPass.render(WIN_WIDTH, WIN_HEIGHT, camera, lightPos, geometryPass.getPositionTexture(), geometryPass.getNormalTexture(), geometryPass.getColorTexture());
            
            // final pass
            finalPass.render(WIN_WIDTH, WIN_HEIGHT, lightingPass.getLightingTexture());
            
            // lidar sensor visualization
            lidarSensor.renderVisualization(viewMatrix, projMatrix, WIN_WIDTH, WIN_HEIGHT, geometryPass.getDepthTexture());
            
            window.swapBuffers();
            window.pollEvents();
        }

        Renderer::Window::terminateGLFW();
        LOG_INFO("Quit 3DGS Renderer");
        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        Renderer::Window::terminateGLFW();
        return -1;
    }
}