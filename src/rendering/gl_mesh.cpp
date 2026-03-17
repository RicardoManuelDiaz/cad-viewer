#include "gl_mesh.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

GLMesh uploadRenderMesh(const RenderMesh& mesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        throw std::runtime_error("Cannot upload an empty RenderMesh.");
    }

    GLMesh out{};
    out.indexCount = static_cast<GLsizei>(mesh.indices.size());

    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);
    glGenBuffers(1, &out.ebo);

    if (out.vao == 0 || out.vbo == 0 || out.ebo == 0)
    {
        destroyGLMesh(out);
        throw std::runtime_error("Failed to create OpenGL mesh buffers.");
    }

    glBindVertexArray(out.vao);

    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
        mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );

    // location 0 = position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, px))
    );

    // location 1 = normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, nx))
    );

    glBindVertexArray(0);

    return out;
}

void drawGLMesh(const GLMesh& mesh)
{
    if (mesh.vao == 0 || mesh.indexCount <= 0)
    {
        return;
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void destroyGLMesh(GLMesh& mesh)
{
    if (mesh.ebo != 0)
    {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }

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

    mesh.indexCount = 0;
}