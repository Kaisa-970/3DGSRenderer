#pragma once

#include "Core/RenderCore.h"
#include <cmath>

RENDERER_NAMESPACE_BEGIN

float sigmoid(const float m1)
{
	return 1.0f / (1.0f + std::exp(-m1));
}

float inverse_sigmoid(const float m1)
{
	return std::log(m1 / (1.0f - m1));
}

RENDERER_NAMESPACE_END