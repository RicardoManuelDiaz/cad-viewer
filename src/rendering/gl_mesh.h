#pragma once

#include <glad/glad.h>

#include "core/render_types.h"

struct GLMesh
{
    GLuint vao{};
    GLuint vbo{};
    GLuint ebo{};
    GLsizei indexCount{};
};

GLMesh uploadRenderMesh(const RenderMesh& mesh);
void drawGLMesh(const GLMesh& mesh);
void destroyGLMesh(GLMesh& mesh);