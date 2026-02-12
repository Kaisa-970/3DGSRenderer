#include "Camera.h"
#include "MathUtils/Vector.h"
#include <algorithm>
#include <iostream>
#include <cmath>
RENDERER_NAMESPACE_BEGIN

// 常量
static const float PI = 3.14159265358979323846f;
static const float DEG_TO_RAD = PI / 180.0f;

// 构造函数
Camera::Camera(const Vector3 &position, const Vector3 &worldUp, float yaw, float pitch)
    : position_(position), worldUp_(worldUp), yaw_(yaw), pitch_(pitch), movementSpeed_(2.5f), mouseSensitivity_(0.1f),
      fov_(45.0f)
{
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : position_(posX, posY, posZ), worldUp_(upX, upY, upZ), yaw_(yaw), pitch_(pitch), movementSpeed_(2.5f),
      mouseSensitivity_(0.1f), fov_(45.0f)
{
    updateCameraVectors();
}

void Camera::lookAt(const Vector3 &target)
{
    // 计算从相机到目标的方向向量
    Vector3 direction = VectorUtils::Normalize(target - position_);

    // 计算俯仰角 (pitch)
    pitch_ = std::asin(direction.y) * 180.0f / PI;

    // 计算偏航角 (yaw)
    yaw_ = std::atan2(direction.x, direction.z) * 180.0f / PI;

    // 更新相机向量
    updateCameraVectors();
}

void Camera::lookAt(float x, float y, float z)
{
    lookAt(Vector3(x, y, z));
}

// 更新相机向量
void Camera::updateCameraVectors()
{
    // 计算新的 Front 向量
    float yawRad = yaw_ * DEG_TO_RAD;
    float pitchRad = pitch_ * DEG_TO_RAD;

    front_.x = std::cos(yawRad) * std::cos(pitchRad);
    front_.y = std::sin(pitchRad);
    front_.z = std::sin(yawRad) * std::cos(pitchRad);
    front_ = VectorUtils::Normalize(front_);

    // 计算 Right 和 Up 向量
    right_ = VectorUtils::Normalize(VectorUtils::Cross(front_, worldUp_));
    up_ = VectorUtils::Normalize(VectorUtils::Cross(right_, front_));
}

// 获取视图矩阵（返回Matrix4）
Mat4 Camera::getViewMatrix() const
{
    return Mat4::LookAt(position_, position_ + front_, up_);
}

// 透视投影矩阵（返回Matrix4）
Mat4 Camera::getPerspectiveMatrix(float fov, float aspect, float near, float far) const
{
    return Mat4::Perspective(fov * DEG_TO_RAD, aspect, near, far);
}

// 正交投影矩阵（返回Matrix4）
Mat4 Camera::getOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) const
{
    return Mat4::Ortho(left, right, bottom, top, near, far);
}

// 兼容旧接口 - 输出到float数组（列主序）
void Camera::getViewMatrix(float *matrix) const
{
    Mat4 view = getViewMatrix();
    memcpy(matrix, view.data(), sizeof(float) * 16);
}

void Camera::getPerspectiveMatrix(float *matrix, float fov, float aspect, float near, float far) const
{
    Mat4 proj = getPerspectiveMatrix(fov, aspect, near, far);
    memcpy(matrix, proj.data(), sizeof(float) * 16);
}

void Camera::getOrthographicMatrix(float *matrix, float left, float right, float bottom, float top, float near,
                                   float far) const
{
    Mat4 ortho = getOrthographicMatrix(left, right, bottom, top, near, far);
    memcpy(matrix, ortho.data(), sizeof(float) * 16);
}

// 键盘移动
void Camera::processKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = movementSpeed_ * deltaTime;

    switch (direction)
    {
    case CameraMovement::Forward:
        position_ += front_ * velocity;
        break;
    case CameraMovement::Backward:
        position_ -= front_ * velocity;
        break;
    case CameraMovement::Left:
        position_ -= right_ * velocity;
        break;
    case CameraMovement::Right:
        position_ += right_ * velocity;
        break;
    case CameraMovement::Up:
        position_ += up_ * velocity;
        break;
    case CameraMovement::Down:
        position_ -= up_ * velocity;
        break;
    }
}

// 鼠标移动
void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= mouseSensitivity_;
    yOffset *= mouseSensitivity_;
    yaw_ += xOffset;
    pitch_ += yOffset;

    // 限制俯仰角
    if (constrainPitch)
    {
        pitch_ = std::max(-89.9f, std::min(89.9f, pitch_));
    }

    updateCameraVectors();
}

// 鼠标滚轮
void Camera::processMouseScroll(float yOffset)
{
    fov_ -= yOffset;
    fov_ = std::max(1.0f, std::min(45.0f, fov_));
}

// Getter 实现（兼容旧接口）
void Camera::getPosition(float &x, float &y, float &z) const
{
    x = position_.x;
    y = position_.y;
    z = position_.z;
}

void Camera::getFront(float &x, float &y, float &z) const
{
    x = front_.x;
    y = front_.y;
    z = front_.z;
}

void Camera::getUp(float &x, float &y, float &z) const
{
    x = up_.x;
    y = up_.y;
    z = up_.z;
}

void Camera::getRight(float &x, float &y, float &z) const
{
    x = right_.x;
    y = right_.y;
    z = right_.z;
}

// Setter 实现
void Camera::setPosition(const Vector3 &position)
{
    position_ = position;
}

void Camera::setPosition(float x, float y, float z)
{
    position_.x = x;
    position_.y = y;
    position_.z = z;
}

void Camera::setFov(float fov)
{
    fov_ = std::max(1.0f, std::min(45.0f, fov));
}

RENDERER_NAMESPACE_END
