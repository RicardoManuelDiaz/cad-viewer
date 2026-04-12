#include "gl_edge_quads.h"
#include <cstddef>

bool uploadEdgeQuadMesh(const EdgeQuadMeshCpu& cpu, GLEdgeQuadMesh& gpu)
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
        static_cast<GLsizeiptr>(cpu.vertices.size() * sizeof(EdgeQuadVertex)),
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

    // location 0 : clip-space position (vec4)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE,
        sizeof(EdgeQuadVertex),
        reinterpret_cast<void*>(offsetof(EdgeQuadVertex, clipX))
    );

    // location 1 : owner edge id (uint)
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(
        1, 1, GL_UNSIGNED_INT,
        sizeof(EdgeQuadVertex),
        reinterpret_cast<void*>(offsetof(EdgeQuadVertex, ownerEdgeId))
    );

    // location 2 : coverage (-1 or +1 at edges)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE,
        sizeof(EdgeQuadVertex),
        reinterpret_cast<void*>(offsetof(EdgeQuadVertex, coverage))
    );

    glBindVertexArray(0);

    gpu.indexCount = static_cast<GLsizei>(cpu.indices.size());
    return true;
}

void drawEdgeQuadMesh(const GLEdgeQuadMesh& mesh)
{
    if (mesh.vao == 0 || mesh.indexCount <= 0) return;

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void destroyEdgeQuadMesh(GLEdgeQuadMesh& mesh)
{
    if (mesh.ebo != 0) { glDeleteBuffers(1, &mesh.ebo); mesh.ebo = 0; }
    if (mesh.vbo != 0) { glDeleteBuffers(1, &mesh.vbo); mesh.vbo = 0; }
    if (mesh.vao != 0) { glDeleteVertexArrays(1, &mesh.vao); mesh.vao = 0; }
    mesh.indexCount = 0;
}
