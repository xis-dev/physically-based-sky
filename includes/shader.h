#pragma once

#include <optional>
#include <string>

#include "vec2.hpp"
#include "vec3.hpp"
#include "glm/mat4x4.hpp"


class Shader {
private:
    enum ObjectType {
        Program, Vertex, Fragment, Geometry
    };

    unsigned m_id{};
    static void errorCheck(unsigned object, ObjectType type);

    static std::optional<std::string> tryGetShaderSource(const char* fileName, ObjectType type);

public:
    Shader() = default;
    Shader(const char* vertexFile, const char* fragmentFile);
    Shader(const char* vertexFile, const char* fragmentFile, const char* geometryFile);

    Shader(const Shader& s) = delete;
    Shader operator=(const Shader& s) = delete;

    Shader(Shader&& s) noexcept : m_id(s.m_id)
    {
        s.m_id = 0;
    }

    Shader& operator=(Shader&& s) noexcept
    {
        if (this != &s)
        {
            destroy();
            m_id = s.m_id;
            s.m_id = 0;
        }

        return *this;
    }

    void setUniformi(const char* name, int value) const;
    void setUniformf(const char* name, float value) const;
    void setUniformVec3(const char* name, const glm::vec3& value) const;
    void setUniformVec2(const char* name, const glm::vec2& value) const;
    void setUniformMat4(const char* name, const glm::mat4& value) const;
    void setUniformBlock(const char* name, int idx);
    void use() const;
    unsigned get() const;
    void destroy();


    ~Shader();

};
