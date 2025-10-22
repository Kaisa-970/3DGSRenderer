#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <stdexcept>

RENDERER_NAMESPACE_BEGIN

static void checkShaderCompile(GLuint shader, const char *stageName) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog;
        infoLog.resize(static_cast<size_t>(logLength > 1 ? logLength : 1));
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
        std::cerr << "[Shader Compile Error] Stage: " << stageName << "\n" << infoLog << std::endl;
        throw std::runtime_error("Shader compilation failed");
    }
}

static void checkProgramLink(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog;
        infoLog.resize(static_cast<size_t>(logLength > 1 ? logLength : 1));
        glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
        std::cerr << "[Program Link Error]\n" << infoLog << std::endl;
        throw std::runtime_error("Program linking failed");
    }
}

unsigned int Shader::compileStage(int type, const std::string &source) {
    GLenum glType = type == 0 ? GL_VERTEX_SHADER : (type == 1 ? GL_FRAGMENT_SHADER : GL_INVALID_ENUM);
    GLuint shader = glCreateShader(glType);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    const char *stageName = glType == GL_VERTEX_SHADER ? "VERTEX" : (type == GL_FRAGMENT_SHADER ? "FRAGMENT" : "INVALID_ENUM");
    checkShaderCompile(shader, stageName);
    return shader;
}

Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource) {
    GLuint vs = compileStage(0, vertexSource);
    GLuint fs = compileStage(1, fragmentSource);

    programId_ = glCreateProgram();
    glAttachShader(programId_, vs);
    glAttachShader(programId_, fs);
    glLinkProgram(programId_);

    // Shaders can be deleted after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    checkProgramLink(programId_);
}

Shader::~Shader() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
        programId_ = 0;
    }
}

Shader::Shader(Shader &&other) noexcept {
    programId_ = other.programId_;
    other.programId_ = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
    if (this != &other) {
        if (programId_ != 0) {
            glDeleteProgram(programId_);
        }
        programId_ = other.programId_;
        other.programId_ = 0;
    }
    return *this;
}

void Shader::use() const {
    glUseProgram(programId_);
}

RENDERER_NAMESPACE_END


