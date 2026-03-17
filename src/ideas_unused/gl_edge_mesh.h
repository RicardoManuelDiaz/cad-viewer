#pragma once

/*#include <glad/glad.h>

#include "render_types.h"

struct GLEdgeMesh
{
    GLuint vao{};
    GLuint vbo{};
    GLsizei vertexCount{};
};

GLEdgeMesh uploadEdgeMesh(const EdgeMesh& mesh);
void drawEdgeMesh(const GLEdgeMesh& mesh);
void destroyEdgeMesh(GLEdgeMesh& mesh);*/

#include "render_types.h"

#include <glad/glad.h>

struct GLEdgeMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

bool uploadEdgeMesh(const EdgeQuadMeshCpu& cpu, GLEdgeMesh& gpu);
void drawEdgeMesh(const GLEdgeMesh& mesh);
void destroyEdgeMesh(GLEdgeMesh& mesh);