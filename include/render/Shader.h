#pragma once

#include <string>
#include <cstdint>

namespace sculpt {

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);
    void use() const;
    void destroy();

    uint32_t getProgram() const { return m_program; }

    // Uniform setters
    void setMat4(const std::string& name, const float* value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

    bool isValid() const { return m_program != 0; }

private:
    uint32_t m_program = 0;
    uint32_t m_vertShader = 0;
    uint32_t m_fragShader = 0;
};

} // namespace sculpt
