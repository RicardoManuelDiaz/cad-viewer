#include "gl_edge_impostor.h"

#include <cstddef>

bool uploadEdgeImpostorMesh(const EdgeImpostorMeshCpu& cpu, GLEdgeImpostorMesh& gpu)
{
    if (cpu.empty())
    {
        gpu.indexCount = 0;
        return false;
    }

    if (gpu.vao == 0) glGenVertexArrays(1, &gpu.vao);
    if (gpu.vbo == 0) glGenBuffers(1, &gpu.vbo);
    if (gpu.ebo == 0) glGenBuffers(1, &gpu.ebo);

    glBindVertexArray(gpu.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(cpu.vertices.size() * sizeof(EdgeImpostorVertex)),
        cpu.vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(cpu.indices.size() * sizeof(std::uint32_t)),
        cpu.indices.data(),
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, clipX)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, lineAxPx)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, lineBxPx)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, startNxPx)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, endNxPx)));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        5, 1, GL_FLOAT, GL_FALSE, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, halfWidthPx)));

    glEnableVertexAttribArray(6);
    glVertexAttribIPointer(
        6, 1, GL_UNSIGNED_INT, sizeof(EdgeImpostorVertex),
        reinterpret_cast<void*>(offsetof(EdgeImpostorVertex, ownerEdgeId)));

    glBindVertexArray(0);

    gpu.indexCount = static_cast<GLsizei>(cpu.indices.size());
    return true;
}

void drawEdgeImpostorMesh(const GLEdgeImpostorMesh& mesh)
{
    if (mesh.vao == 0 || mesh.indexCount <= 0)
        return;

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void destroyEdgeImpostorMesh(GLEdgeImpostorMesh& mesh)
{
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    mesh = {};
}