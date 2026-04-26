#include "includes/Shader.h"

#include "includes/PathResolver.h"


#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <optional>





Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
	const unsigned vertex = glCreateShader(GL_VERTEX_SHADER);
	const unsigned fragment = glCreateShader(GL_FRAGMENT_SHADER);

	const auto str_vertexShaderSource = tryGetShaderSource(vertexFile, Vertex);
	const auto str_fragmentShaderSource = tryGetShaderSource(fragmentFile, Fragment);

	if (str_vertexShaderSource && str_fragmentShaderSource)
	{

		const char* vertexSource = str_vertexShaderSource.value().c_str();
		const char* fragmentSource = str_fragmentShaderSource.value().c_str();

		glShaderSource(vertex, 1, &vertexSource, nullptr);
		glShaderSource(fragment, 1, &fragmentSource, nullptr);

		glCompileShader(vertex);
		errorCheck(vertex, ObjectType::Vertex);
		glCompileShader(fragment);
		errorCheck(fragment, ObjectType::Fragment);

		m_id = glCreateProgram();
		glAttachShader(m_id, vertex);
		glAttachShader(m_id, fragment);

		glLinkProgram(m_id);
		errorCheck(m_id, ObjectType::Program);
	}
	else
	{
		std::cout << "SHADER:: Failed to get shader source.\n";
		return;
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

Shader::Shader(const char* vertexFile, const char* fragmentFile, const char* geometryFile)
{
	const unsigned vertex = glCreateShader(GL_VERTEX_SHADER);
	const unsigned fragment = glCreateShader(GL_FRAGMENT_SHADER);
	const unsigned geometry = glCreateShader(GL_GEOMETRY_SHADER);

	const auto str_vertexShaderSource = tryGetShaderSource(vertexFile, Vertex);
	const auto str_fragmentShaderSource = tryGetShaderSource(fragmentFile, Fragment);
	const auto str_geoShaderSource = tryGetShaderSource(geometryFile, Geometry);

	if (str_vertexShaderSource && str_fragmentShaderSource && str_geoShaderSource)
	{

		const char* vertexSource = str_vertexShaderSource.value().c_str();
		const char* fragmentSource = str_fragmentShaderSource.value().c_str();
		const char* geoSource = str_geoShaderSource.value().c_str();

		glShaderSource(vertex, 1, &vertexSource, nullptr);
		glShaderSource(fragment, 1, &fragmentSource, nullptr);
		glShaderSource(geometry, 1, &geoSource, nullptr);

		glCompileShader(vertex);
		errorCheck(vertex, ObjectType::Vertex);
		glCompileShader(fragment);
		errorCheck(fragment, ObjectType::Fragment);
		glCompileShader(geometry);
		errorCheck(geometry, ObjectType::Geometry);

		m_id = glCreateProgram();
		glAttachShader(m_id, vertex);
		glAttachShader(m_id, fragment);
		glAttachShader(m_id, geometry);

		glLinkProgram(m_id);
		errorCheck(m_id, ObjectType::Program);
	}
	else
	{
		std::cout << "SHADER:: Failed to get shader source.\n";
		return;
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);
	glDeleteShader(geometry);
}

std::optional<std::string> Shader::tryGetShaderSource(const char* fileName, ObjectType type)
{
	const std::string filePath = PathResolver::getPath(fileName);
	if (!PathResolver::fileExists(filePath))
	{
		switch (type)
		{
		case ObjectType::Vertex:
			std::cout << "SHADER:: Vertex shader file does not exist.\n";
			break;

		case ObjectType::Fragment:
			std::cout << "SHADER:: Fragment shader file does not exist.\n";
			break;
		case Geometry:
			std::cout << "SHADER:: Geometry shader file does not exist.\n";
			break;
		default:
			std::cout << "SHADER:: Shader file does not exist.\n";
		}

		return {};
	}

	const std::ifstream stream(filePath);
	if (!stream.is_open())
	{
		std::cout << "FAILED TO OPEN FILE. filename: " << fileName << "\n";
		return {};
	}
	std::stringstream string;
	string << stream.rdbuf();
	return string.str();
}

void Shader::setUniformi(const char* name, int value) const
{
	auto uniformLoc = glGetUniformLocation(m_id, name);
	glUseProgram(m_id);
	glUniform1i(uniformLoc, value);
}

void Shader::setUniformf(const char* name, float value) const
{
	auto uniformLoc = glGetUniformLocation(m_id, name);
	glUseProgram(m_id);
	glUniform1f(uniformLoc, value);
}


void Shader::setUniformVec3(const char* name, const glm::vec3& value) const
{
	auto uniformLoc = glGetUniformLocation(m_id, name);
	glUniform3f(uniformLoc, value.x, value.y, value.z);
}

void Shader::setUniformVec2(const char* name, const glm::vec2& value) const
{
	auto uniformLoc = glGetUniformLocation(m_id, name);
	glUniform3fv(uniformLoc, 1, glm::value_ptr(value));
}

void Shader::setUniformMat4(const char* name, const glm::mat4& value) const
{
	auto uniformLoc = glGetUniformLocation(m_id, name);
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniformBlock(const char* name, int idx)
{
	auto blockIdx = glGetUniformBlockIndex(m_id, name);
	glUniformBlockBinding(m_id, blockIdx, idx);
}


void Shader::use() const
{
	glUseProgram(m_id);
}

unsigned Shader::get() const
{
	return m_id;
}

void Shader::destroy()
{
	if (m_id != 0) glDeleteProgram(m_id);
}

Shader::~Shader()
{
	destroy();
}



void Shader::errorCheck(unsigned object, ObjectType type)
{
	int success;
	char infoLog[512];

	switch (type)
	{
	case ObjectType::Program:
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(object, 512, nullptr, infoLog);
			std::cout << "ERROR lINKING SHADER PROGRAM. \n" << infoLog;
		}
		break;

	case ObjectType::Vertex:
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, nullptr, infoLog);
			std::cout << "ERROR COMPILING VERTEX SHADER. \n" << infoLog;
		}
		break;

	case ObjectType::Fragment:
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, nullptr, infoLog);
			std::cout << "ERROR COMPILING FRAGMENT SHADER. \n" << infoLog;
		}
		break;

	case ObjectType::Geometry:
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, nullptr, infoLog);
			std::cout << "ERROR COMPILING GEOMETRY SHADER. \n" << infoLog;
		}
		break;
	}
}


