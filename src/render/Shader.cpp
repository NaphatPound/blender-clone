#include "render/Shader.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>
#include <vector>

namespace sculpt {

Shader::~Shader() {
    destroy();
}

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
    // Compile vertex shader
    m_vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vSrc = vertexSrc.c_str();
    glShaderSource(m_vertShader, 1, &vSrc, nullptr);
    glCompileShader(m_vertShader);

    int success;
    glGetShaderiv(m_vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(m_vertShader, 512, nullptr, log);
        std::cerr << "Vertex shader error: " << log << std::endl;
        return false;
    }

    // Compile fragment shader
    m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSrc = fragmentSrc.c_str();
    glShaderSource(m_fragShader, 1, &fSrc, nullptr);
    glCompileShader(m_fragShader);

    glGetShaderiv(m_fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(m_fragShader, 512, nullptr, log);
        std::cerr << "Fragment shader error: " << log << std::endl;
        return false;
    }

    // Link program
    m_program = glCreateProgram();
    glAttachShader(m_program, m_vertShader);
    glAttachShader(m_program, m_fragShader);
    glLinkProgram(m_program);

    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_program, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
        return false;
    }

    // Clean up individual shaders
    glDeleteShader(m_vertShader);
    glDeleteShader(m_fragShader);
    m_vertShader = 0;
    m_fragShader = 0;

    return true;
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::setMat4(const std::string& name, const float* value) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), 1, GL_FALSE, value);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(m_program, name.c_str()), x, y, z);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), value);
}

} // namespace sculpt
