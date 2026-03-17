#include "shader_utils.h"

#include <stdexcept>
#include <string>
#include <vector>

GLuint compileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        throw std::runtime_error("glCreateShader failed.");
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<GLchar> log(static_cast<size_t>(logLength > 1 ? logLength : 1));
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());

        const std::string stage =
            (type == GL_VERTEX_SHADER) ? "vertex" :
            (type == GL_FRAGMENT_SHADER) ? "fragment" :
            "unknown";

        glDeleteShader(shader);
        throw std::runtime_error("Failed to compile " + stage + " shader:\n" + std::string(log.data()));
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    const GLuint vertexShader   = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    const GLuint program = glCreateProgram();
    if (program == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("glCreateProgram failed.");
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<GLchar> log(static_cast<size_t>(logLength > 1 ? logLength : 1));
        glGetProgramInfoLog(program, logLength, nullptr, log.data());

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);

        throw std::runtime_error("Failed to link shader program:\n" + std::string(log.data()));
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}