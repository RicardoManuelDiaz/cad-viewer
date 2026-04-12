#pragma once

#include "core/render_types.h"
#include <glad/glad.h>

struct GLEdgeQuadMesh
{
    GLuint  vao = 0;
    GLuint  vbo = 0;
    GLuint  ebo = 0;
    GLsizei indexCount = 0;
};

bool uploadEdgeQuadMesh(const EdgeQuadMeshCpu& cpu, GLEdgeQuadMesh& gpu);
void drawEdgeQuadMesh(const GLEdgeQuadMesh& mesh);
void destroyEdgeQuadMesh(GLEdgeQuadMesh& mesh);
