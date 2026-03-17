#pragma once

#include "edge_line_builder.h"

#include <glad/glad.h>
#include <vector>

struct GLEdgeLineMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
    std::vector<EdgeLineRange> ranges;
};

bool uploadEdgeLineMesh(const EdgeLineMeshCpu& cpu, GLEdgeLineMesh& gpu);
void drawEdgeLineMesh(const GLEdgeLineMesh& mesh);
void destroyEdgeLineMesh(GLEdgeLineMesh& mesh);