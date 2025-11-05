#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <stdexcept>
#include <fstream>

RENDERER_NAMESPACE_BEGIN

std::string Shader::readFile(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }
    
    // 读取整个文件
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();
    
    return content;
}

Shader Shader::fromFiles(const std::string &vertexPath, const std::string &fragmentPath) {
    std::string vertexSource = readFile(vertexPath);
    std::string fragmentSource = readFile(fragmentPath);
    return Shader(vertexSource, fragmentSource);
}

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

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::setMat4(const char* name, const float* value) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }
}

void Shader::setVec2(const char* name, float x, float y) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void Shader::setVec3(const char* name, float x, float y, float z) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void Shader::setVec4(const char* name, float x, float y, float z, float w) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void Shader::setFloat(const char* name, float value) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setInt(const char* name, int value) const {
    use();
    int location = glGetUniformLocation(programId_, name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

RENDERER_NAMESPACE_END


