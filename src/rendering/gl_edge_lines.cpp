#include "gl_edge_lines.h"

#include <cstddef>

bool uploadEdgeLineMesh(const EdgeLineMeshCpu& cpu, GLEdgeLineMesh& gpu)
{
    if (cpu.empty())
    {
        gpu.vertexCount = 0;
        gpu.ranges.clear();
        return false;
    }

    if (gpu.vao == 0)
        glGenVertexArrays(1, &gpu.vao);
    if (gpu.vbo == 0)
        glGenBuffers(1, &gpu.vbo);

    glBindVertexArray(gpu.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(cpu.vertices.size() * sizeof(EdgeLineVertex)),
        cpu.vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(EdgeLineVertex),
        reinterpret_cast<void*>(offsetof(EdgeLineVertex, clipX))
    );

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(
        1,
        1,
        GL_UNSIGNED_INT,
        sizeof(EdgeLineVertex),
        reinterpret_cast<void*>(offsetof(EdgeLineVertex, ownerEdgeId))
    );

    glBindVertexArray(0);

    gpu.vertexCount = static_cast<GLsizei>(cpu.vertices.size());
    gpu.ranges = cpu.ranges;
    return true;
}

void drawEdgeLineMesh(const GLEdgeLineMesh& mesh)
{
    if (mesh.vao == 0 || mesh.vertexCount <= 0)
        return;

    glBindVertexArray(mesh.vao);

    for (const EdgeLineRange& r : mesh.ranges)
    {
        if (r.count < 2)
            continue;

        glDrawArrays(r.closed ? GL_LINE_LOOP : GL_LINE_STRIP, r.first, r.count);
    }

    glBindVertexArray(0);
}

void destroyEdgeLineMesh(GLEdgeLineMesh& mesh)
{
    if (mesh.vbo != 0)
    {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }

    if (mesh.vao != 0)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }

    mesh.vertexCount = 0;
    mesh.ranges.clear();
}