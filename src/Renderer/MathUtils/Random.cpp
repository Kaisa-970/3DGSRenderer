#include "Random.h"
#include <random>

RENDERER_NAMESPACE_BEGIN

static std::random_device rd;
static std::mt19937 gen(rd());

float Random::randomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

int Random::randomInt(int min, int max)
{
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

Vector3 Random::randomVector3(float min, float max)
{
    return Vector3(
        randomFloat(min, max),
        randomFloat(min, max),
        randomFloat(min, max)
    );
}

Vector3 Random::randomColor()
{
    // 生成 [0, 1] 范围的随机颜色
    return Vector3(
        randomFloat(0.0f, 1.0f),
        randomFloat(0.0f, 1.0f),
        randomFloat(0.0f, 1.0f)
    );
}

RENDERER_NAMESPACE_END