#pragma once

#include "Core/RenderCore.h"
#include <string>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Shader {
public:
    Shader(const std::string &vertexSource, const std::string &fragmentSource);
    // 从文件路径构造
    static Shader fromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;

    void use() const;
    unsigned int id() const { return programId_; }

    void setMat4(const char* name, const float* value) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;
private:
    unsigned int programId_ = 0;

    static unsigned int compileStage(int type, const std::string &source);
    static std::string readFile(const std::string &filepath);
};

RENDERER_NAMESPACE_END


