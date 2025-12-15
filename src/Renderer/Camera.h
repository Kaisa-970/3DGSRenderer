#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Matrix.h"
#include "MathUtils/Vector.h"

RENDERER_NAMESPACE_BEGIN

enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

class RENDERER_API Camera
{
public:
    // 构造函数
    Camera(const Vector3 &position = Vector3(0.0f, 0.0f, 3.0f), const Vector3 &worldUp = Vector3(0.0f, 1.0f, 0.0f),
           float yaw = 0.0f, float pitch = 0.0f);

    Camera(float posX, float posY, float posZ, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f, float yaw = 0.0f,
           float pitch = 0.0f);

    // 让相机看向指定目标点
    void lookAt(const Vector3 &target);
    void lookAt(float x, float y, float z);

    // 获取矩阵（返回Matrix4对象）
    Matrix4 getViewMatrix() const;
    Matrix4 getPerspectiveMatrix(float fov, float aspect, float near, float far) const;
    Matrix4 getOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) const;

    // 兼容旧接口（输出到float数组，列主序）
    void getViewMatrix(float *matrix) const;
    void getPerspectiveMatrix(float *matrix, float fov, float aspect, float near, float far) const;
    void getOrthographicMatrix(float *matrix, float left, float right, float bottom, float top, float near,
                               float far) const;

    // 相机移动
    void processKeyboard(CameraMovement direction, float deltaTime);

    // 鼠标移动（控制旋转）
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    // 鼠标滚轮（控制 FOV 缩放）
    void processMouseScroll(float yOffset);

    // Getter 方法（返回Vector3）
    const Vector3 &getPosition() const
    {
        return position_;
    }
    const Vector3 &getFront() const
    {
        return front_;
    }
    const Vector3 &getUp() const
    {
        return up_;
    }
    const Vector3 &getRight() const
    {
        return right_;
    }

    // 兼容旧接口
    void getPosition(float &x, float &y, float &z) const;
    void getFront(float &x, float &y, float &z) const;
    void getUp(float &x, float &y, float &z) const;
    void getRight(float &x, float &y, float &z) const;

    float getYaw() const
    {
        return yaw_;
    }
    float getPitch() const
    {
        return pitch_;
    }
    float getFov() const
    {
        return fov_;
    }
    bool isChanged() const
    {
        return isChanged_;
    }

    // Setter 方法
    void setPosition(const Vector3 &position);
    void setPosition(float x, float y, float z);
    void setMovementSpeed(float speed)
    {
        movementSpeed_ = speed;
    }
    void setMouseSensitivity(float sensitivity)
    {
        mouseSensitivity_ = sensitivity;
    }
    void setFov(float fov);

private:
    // 相机属性（使用Vector3）
    Vector3 position_; // 位置
    Vector3 front_;    // 前方向
    Vector3 up_;       // 上方向
    Vector3 right_;    // 右方向
    Vector3 worldUp_;  // 世界上方向

    // 欧拉角
    float yaw_;   // 偏航角（左右旋转）
    float pitch_; // 俯仰角（上下旋转）

    // 相机选项
    float movementSpeed_;
    float mouseSensitivity_;
    float fov_;             // 视野角度
    bool isChanged_{false}; // 相机是否被改变

    // 更新相机向量
    void updateCameraVectors();
};

RENDERER_NAMESPACE_END
