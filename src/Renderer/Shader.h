#pragma once

#include "Core/RenderCore.h"
#include <string>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Shader {
public:
    Shader(const std::string &vertexSource, const std::string &fragmentSource);
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;

    void use() const;
    unsigned int id() const { return programId_; }

private:
    unsigned int programId_ = 0;

    static unsigned int compileStage(int type, const std::string &source);
};

RENDERER_NAMESPACE_END


