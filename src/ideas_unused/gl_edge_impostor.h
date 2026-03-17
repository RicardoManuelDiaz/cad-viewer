#pragma once

#include "edge_impostor_builder.h"

#include <glad/glad.h>

struct GLEdgeImpostorMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

bool uploadEdgeImpostorMesh(const EdgeImpostorMeshCpu& cpu, GLEdgeImpostorMesh& gpu);
void drawEdgeImpostorMesh(const GLEdgeImpostorMesh& mesh);
void destroyEdgeImpostorMesh(GLEdgeImpostorMesh& mesh);