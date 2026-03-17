#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "viewer_gl.h"

#include "camera.h"
#include "math3d.h"
#include "mesh_bounds.h"
#include "render_types.h"

#include "mesh_edges.h"
#include "mesh_prep.h"
#include "occt_edges.h"
#include "shape_gen.h"
#include "shape_topology.h"

#include "edge_line_builder.h"
#include "gl_edge_lines.h"
#include "gl_mesh.h"
#include "shader_utils.h"

#include <vector>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>

static Camera g_camera;
static bool g_middleMouseDown = false;
static bool g_ctrlPanMode = false;

static void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
    g_camera.setViewport(width, height);
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (button != GLFW_MOUSE_BUTTON_MIDDLE)
    {
        return;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    const bool ctrlDown =
        (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    if (action == GLFW_PRESS)
    {
        g_middleMouseDown = true;
        g_ctrlPanMode = ctrlDown;

        if (g_ctrlPanMode)
        {
            g_camera.beginPan(x, y);
        }
        else
        {
            g_camera.beginRotate(x, y);
        }
    }
    else if (action == GLFW_RELEASE)
    {
        g_middleMouseDown = false;

        if (g_ctrlPanMode)
        {
            g_camera.endPan();
        }
        else
        {
            g_camera.endRotate();
        }

        g_ctrlPanMode = false;
    }
}

static void cursor_position_callback(GLFWwindow* /*window*/, double x, double y)
{
    if (!g_middleMouseDown)
    {
        return;
    }

    if (g_ctrlPanMode)
    {
        g_camera.updatePan(x, y);
    }
    else
    {
        g_camera.updateRotate(x, y);
    }
}

static void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    g_camera.zoomWheel(x, y, yoffset);
}

static const char* kVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vViewPos;
out vec3 vViewNormal;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vec4 viewPos  = uView * worldPos;

    mat3 normalMat = mat3(transpose(inverse(uView * uModel)));

    vViewPos    = viewPos.xyz;
    vViewNormal = normalize(normalMat * aNormal);

    gl_Position = uProj * viewPos;
}
)";

static const char* kFragmentShaderSource = R"(
#version 330 core

in vec3 vViewPos;
in vec3 vViewNormal;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vViewNormal);
    vec3 V = normalize(-vViewPos);

    float head = max(dot(N, V), 0.0);

    // Bias toward a broad frontal side highlight
    //float frontal = pow(max(head, 0.0), 0.65);

    float hemi = 0.5 * N.y + 0.5;

    //vec3 baseColor = vec3(0.79, 0.80, 0.83);

    vec3 color =
        vec3(0.7, 0.74, 0.76);

    FragColor = vec4(color, 1.0);
}
)";

static const char* kEdgeVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec4 aClipPos;
layout (location = 1) in uint aOwnerEdgeId;

flat out uint vOwnerEdgeId;

void main()
{
    gl_Position = aClipPos;
    vOwnerEdgeId = aOwnerEdgeId;
}
)";

static const char* kEdgeFragmentShaderSource = R"(
#version 330 core

flat in uint vOwnerEdgeId;

uniform vec4 uEdgeColor;

out vec4 FragColor;

void main()
{
    FragColor = uEdgeColor;
}
)";

int runViewer()
{
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;
    GLuint edgeShaderProgram = 0;
    GLMesh gpuMesh{};
    GLEdgeLineMesh gpuEdgeMesh{};

    try
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        const int startWidth = 2200;
        const int startHeight = 1800;

        window = glfwCreateWindow(startWidth, startHeight, "basic_occt_gl_viewer", nullptr, nullptr);
        if (window == nullptr)
        {
            throw std::runtime_error("Failed to create GLFW window.");
        }

        glfwMakeContextCurrent(window);

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetScrollCallback(window, scroll_callback);

        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            throw std::runtime_error("Failed to initialize GLAD.");
        }

        int fbWidth     = 0;
        int fbHeight    = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        glEnable(GL_DEPTH_TEST);

        g_camera.setViewport(fbWidth, fbHeight);

        // ----------------------------------------------------
        // Create test shape and mesh
        // ----------------------------------------------------
        const TopoDS_Shape shape    = makeExtrudedCircle(20.0, 50.0);
        const RenderMesh mesh       = prepareForOpenGL(shape, 0.15, 0.50);      // linear and angular deflection

        ShapeTopology topology;
        if (!topology.build(shape))
        {
            throw std::runtime_error("Failed to build shape topology.");
        }

        std::vector<EdgePolyline3D> edgePolylines = extractOcctEdgePolylines(topology);
        cleanupEdgePolylines(edgePolylines);

        //std::cout << "Mesh vertices  : " << mesh.vertices.size() << '\n';
        //std::cout << "Mesh triangles : " << (mesh.indices.size() / 3) << '\n';

        const Bounds3 bounds    = computeMeshBounds(mesh);
        const Vec3 center       = computeBoundsCenter(bounds);
        //const float radius      = computeBoundsRadius(bounds, center);

        g_camera.setPivot(center);

        //g_camera.setDistance(radius * 1.0f);
        //g_camera.setYawPitch(0.75f, 0.55f);

        // ----------------------------------------------------
        // Main mesh resources
        // ----------------------------------------------------
        shaderProgram   = createShaderProgram(kVertexShaderSource, kFragmentShaderSource);
        gpuMesh         = uploadRenderMesh(mesh);

        const GLint locModel    = glGetUniformLocation(shaderProgram, "uModel");
        const GLint locView     = glGetUniformLocation(shaderProgram, "uView");
        const GLint locProj     = glGetUniformLocation(shaderProgram, "uProj");

        if (locModel < 0 || locView < 0 || locProj < 0)
        {
            throw std::runtime_error("Failed to find one or more main shader uniforms.");
        }

        // ----------------------------------------------------
        // Render loop
        // ----------------------------------------------------
        while (!glfwWindowShouldClose(window))
        {
            processInput(window);

            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            g_camera.setViewport(fbWidth, fbHeight);

            const Mat4 model = identity();
            const Mat4 view = g_camera.viewMatrix();
            const Mat4 proj = g_camera.projMatrix();

            const Mat4 edgeViewProj = multiply(proj, view);

            const EdgeLineMeshCpu edgeLineCpu =
                buildEdgeLineMesh(edgePolylines, edgeViewProj);

            uploadEdgeLineMesh(edgeLineCpu, gpuEdgeMesh);

            glClearColor(0.93f, 0.93f, 0.93f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);

            glUseProgram(shaderProgram);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
            glUniformMatrix4fv(locView, 1, GL_FALSE, view.m);
            glUniformMatrix4fv(locProj, 1, GL_FALSE, proj.m);

            
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            drawGLMesh(gpuMesh);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(edgeShaderProgram);

            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            drawEdgeLineMesh(gpuEdgeMesh);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        destroyEdgeLineMesh(gpuEdgeMesh);
        destroyGLMesh(gpuMesh);

        if (edgeShaderProgram != 0)
        {
            glDeleteProgram(edgeShaderProgram);
            edgeShaderProgram = 0;
        }

        if (shaderProgram != 0)
        {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';

        destroyEdgeLineMesh(gpuEdgeMesh);
        destroyGLMesh(gpuMesh);

        if (edgeShaderProgram != 0)
        {
            glDeleteProgram(edgeShaderProgram);
            edgeShaderProgram = 0;
        }

        if (shaderProgram != 0)
        {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }

        if (window != nullptr)
        {
            glfwDestroyWindow(window);
        }

        glfwTerminate();
        return -1;
    }
}