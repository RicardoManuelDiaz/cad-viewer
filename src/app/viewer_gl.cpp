#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "viewer_gl.h"

#include "core/camera.h"
#include "core/math3d.h"
#include "core/mesh_bounds.h"
#include "core/render_types.h"

#include "geometry/mesh_edges.h"
#include "geometry/mesh_prep.h"
#include "geometry/occt_edges.h"
#include "geometry/shape_gen.h"
#include "geometry/shape_topology.h"

#include "rendering/edge_line_builder.h"
#include "rendering/gl_edge_lines.h"
#include "rendering/edge_quad_builder.h"
#include "rendering/gl_edge_quads.h"
#include "rendering/gl_mesh.h"
#include "rendering/shader_utils.h"
#include "rendering/font_render.h"
#include "rendering/glfw_fullscreen_controller.h"

#include <vector>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>

static Camera g_camera;
static bool g_middleMouseDown = false;
static bool g_ctrlPanMode = false;
static bool g_prevPKeyDown = false;     // Projection matrix toggle hardcoded to 'P' key for now

// NEW: for shape testing
struct ShapeEntry
{
    GLMesh          gpuMesh{};
    GLEdgeQuadMesh  gpuEdges{};
};

static int  g_activeShape = 0;
static bool g_prevKeyState[9] = {};     // tracks "rising-edge" for keys 1-9

static Vec3 shapeCenter(const TopoDS_Shape& shape)
{
    const RenderMesh m = prepareForOpenGL(shape, 0.5, 1.0);  // coarse — just for bounds
    return computeBoundsCenter(computeMeshBounds(m));
}

static void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
    g_camera.setViewport(width, height);
}

static void processInput(GLFWwindow* window, GlfwFullscreenController& fsController, int numShapes)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Check for fullscreen toggle (rising-edge only)
    static bool s_prevFSToggle = false;
    const bool fsToggleDown = (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS);
    if (fsToggleDown && !s_prevFSToggle)
        fsController.Toggle();
    
    s_prevFSToggle = fsToggleDown;

    // Toggle ortho/perspective on V key (rising-edge only)
    //  - "rising-edge" method means one press = one toggle, regardless of how many frames key is held for
    const bool pDown = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);
    if (pDown && !g_prevPKeyDown)
    {
        g_camera.toggleProjection();
    }
    g_prevPKeyDown = pDown;

    // Keys 1–9 switch shapes (rising-edge only)
    for (int i = 0; i < 9 && i < numShapes; ++i)
    {
        const bool down = (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS);
        if (down && !g_prevKeyState[i])
            g_activeShape = i;
        g_prevKeyState[i] = down;
    }

    // Arrow keys to cycle through test shapes
    const bool rightDown = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
    const bool leftDown = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);

    static bool s_prevRight = false, s_prevLeft = false;

    if (rightDown && !s_prevRight)
        g_activeShape = (g_activeShape + 1) % numShapes;
    if (leftDown && !s_prevLeft)
        g_activeShape = (g_activeShape + numShapes - 1) % numShapes;

    s_prevRight = rightDown;
    s_prevLeft = leftDown;
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

// Direction the light travels (in VIEW space, normalised)
// e.g. (0.4, 0.7, 0.5) means light comes from upper-right-front.
uniform vec3 uLightDir;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vViewNormal);

    //
    // Ambient (hemisphere ambient)
    //  - interpolates between "sky" colour (N pointing up) and "ground" colour (N pointing down)
    float hemi      = 0.5 * N.y + 0.5;
    vec3 ambient    = mix(vec3(0.10, 0.10, 0.13), vec3(0.28, 0.30, 0.35), hemi);

    //
    // Diffuse
    //  - Old-school Lambertian diffuse
    float diff      = max(dot(N, uLightDir), 0.0);      // using max(..., 0) effectively means back-lit faces are ignored
    vec3 diffuse    = diff * vec3(0.72, 0.75, 0.80);

    //
    // Specular
    //  - Blinn-Phong specular highlight
    vec3 V          = normalize(-vViewPos);             // view direction
    vec3 H          = normalize(uLightDir + V);         // half-vector
    float spec      = pow(max(dot(N, H), 0.0), 48.0);
    vec3 specular   = spec * vec3(0.35, 0.35, 0.35);

    //
    // Calculate fragment (pixel) colour and output
    vec3 colour     = ambient + diffuse + specular;

    FragColor = vec4(colour, 1.0);
}
)";

static const char* kEdgeVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec4 aClipPos;
layout (location = 1) in uint aOwnerEdgeId;
layout (location = 2) in float aCoverage;

flat out uint vOwnerEdgeId;
out float vCoverage;

void main()
{
    gl_Position  = aClipPos;
    vOwnerEdgeId = aOwnerEdgeId;
    vCoverage = aCoverage;
}
)";

static const char* kEdgeFragmentShaderSource = R"(
#version 330 core

flat in uint vOwnerEdgeId;
in float vCoverage;

uniform vec4 uEdgeColor;

out vec4 FragColor;

void main()
{
    // For antialiasing... modulates alpha
    // vCoverage is 0 at the line centre and +/- 1 at the quad edges
    // We want full opacity in the centre and a smooth falloff over the outermost ~1 physical pixel
    //
    // smoothstep(edge0, edge1, x): returns 0 when x <= edge0,
    //                               returns 1 when x >= edge1,
    //                               smooth Hermite curve between
    //
    // fwidth(vCoverage) is the sum of the absolute derivatives in x and y — gives roughly "how many coverage units does one pixel span"
    //  - which lets the falloff width stay exactly 1 pixel regardless of line width or zoom level

    float dist      = abs(vCoverage);               // 0 = centre, 1 = edge
    float fw        = fwidth(dist);                 // ~1 pixel in coverage space
    float alpha     = 1.0 - smoothstep(1.0 - fw, 1.0, dist);

    FragColor = vec4(uEdgeColor.rgb, uEdgeColor.a * alpha);
}
)";

int runViewer(const std::string& exePath)
{
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;
    GLuint edgeShaderProgram = 0;

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

        float contentScaleX = 1.0f, contentScaleY = 1.0f;
        glfwGetWindowContentScale(window, &contentScaleX, &contentScaleY);

        // NEW: font support
        const std::string fontPath = exePath + "\\Poppins-Regular.ttf";
        if (!initFontRenderer(fontPath, 30.0f * contentScaleY))
        {
            // Non-fatal — viewer still works without text
            std::cerr << "Warning: could not load font.\n";
        }

        // NEW: fullscreen controller
        GlfwFullscreenController fsController(window);

        int fbWidth     = 0;
        int fbHeight    = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        glEnable(GL_DEPTH_TEST);

        g_camera.setViewport(fbWidth, fbHeight);

        // ----------------------------------------------------
        // Shapes for testing
        // ----------------------------------------------------
        struct ShapeDef
        {
            std::string     label;
            TopoDS_Shape    shape;
        };

        const std::vector<ShapeDef> shapeDefs =
        {
            { "Cylinder",           makeExtrudedCircle(10.0, 10.0)          },
            { "Box",                makeBox(14.0, 14.0, 14.0)               },
            { "Sphere",             makeSphere(10.0)                        },
            { "Cone",               makeCone(8.0, 0.0, 14.0)                },
            { "Truncated cone",     makeCone(5.0, 9.0, 12.0)                },
            { "Torus",              makeTorus(8.0, 3.0)                     },
            { "Hexagonal prism",    makeExtrudedPolygon(6, 10.0, 14.0)      },
            { "Triangular prism",   makeExtrudedPolygon(3, 10.0, 14.0)      },
            { "Hollow cylinder",    makeHollowCylinder(10.0, 6.0, 12.0)     },
            { "Box with hole",      makeBoxWithHole(16.0, 4.0)              },
            { "Revolved profile",   makeRevolvedProfile(4.0, 10.0, 8.0, 2.0)},
            { "Fillet box",         makeFilletBox(16.0, 2.5)                }
        };

        const int numShapes = static_cast<int>(shapeDefs.size());
        
        std::vector<ShapeEntry>              shapeEntries(numShapes);
        std::vector<std::vector<EdgePolyline3D>> shapePolylines(numShapes);

        for (int i = 0; i < numShapes; ++i)
        {
            const TopoDS_Shape& shape = shapeDefs[i].shape;

            const RenderMesh mesh = prepareForOpenGL(shape, 0.1, 0.50);
            shapeEntries[i].gpuMesh = uploadRenderMesh(mesh);

            ShapeTopology topology;
            if (!topology.build(shape))
                throw std::runtime_error("Failed to build topology for: " + shapeDefs[i].label);

            shapePolylines[i] = extractOcctEdgePolylines(topology);
            cleanupEdgePolylines(shapePolylines[i]);

            // Pre-build edge quads at an arbitrary identity viewProj — they get rebuilt every frame anyway, this just allocates the GPU object
            // (Alternatively, defer first upload to the render loop)
        }

        // Use the first shape's bounds to set the initial camera pivot
        {
            const RenderMesh firstMesh = prepareForOpenGL(shapeDefs[0].shape, 0.1, 0.50);
            const Bounds3 bounds = computeMeshBounds(firstMesh);
            const Vec3 center = computeBoundsCenter(bounds);
            g_camera.setPivot(center);
        }

        //g_camera.setDistance(radius * 1.0f);
        //g_camera.setYawPitch(0.75f, 0.55f);

        // ----------------------------------------------------
        // Main mesh resources
        // ----------------------------------------------------
        shaderProgram   = createShaderProgram(kVertexShaderSource, kFragmentShaderSource);

        const GLint locModel    = glGetUniformLocation(shaderProgram, "uModel");
        const GLint locView     = glGetUniformLocation(shaderProgram, "uView");
        const GLint locProj     = glGetUniformLocation(shaderProgram, "uProj");
        const GLint locLightDir = glGetUniformLocation(shaderProgram, "uLightDir");

        if (locModel < 0 || locView < 0 || locProj < 0 || locLightDir < 0)
        {
            throw std::runtime_error("Failed to find one or more main shader uniforms.");
        }

        // NEW: edge shader
        const EdgeStyle edgeStyle{ 3.0f, 4.0f, LineJoin::Miter };  // widthPx, miterLimit, join

        edgeShaderProgram = createShaderProgram(kEdgeVertexShaderSource, kEdgeFragmentShaderSource);

        const GLint locEdgeViewProj = glGetUniformLocation(edgeShaderProgram, "uViewProj");
        const GLint locEdgeColor = glGetUniformLocation(edgeShaderProgram, "uEdgeColor");

        // ----------------------------------------------------
        // Render loop
        // ----------------------------------------------------
        while (!glfwWindowShouldClose(window))
        {
            processInput(window, fsController, numShapes);

            // NEW: handle when a test shape changes, e.g. reset camera pivot
            static int s_lastShape = -1;
            if (g_activeShape != s_lastShape)
            {
                const RenderMesh m = prepareForOpenGL(shapeDefs[g_activeShape].shape, 0.5, 1.0);
                const Vec3 center = computeBoundsCenter(computeMeshBounds(m));
                g_camera.setPivot(center);
                s_lastShape = g_activeShape;
            }

            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            g_camera.setViewport(fbWidth, fbHeight);

            // NEW: for font rendering
            setFontViewport(fbWidth, fbHeight);

            const Mat4 model = identity();
            const Mat4 view = g_camera.viewMatrix();
            const Mat4 proj = g_camera.projMatrix();
            const Mat4 edgeViewProj = multiply(proj, view);

            const EdgeQuadMeshCpu edgeQuadCpu =  buildEdgeQuadMesh(
                shapePolylines[g_activeShape],
                edgeViewProj,
                fbWidth, fbHeight,
                edgeStyle,
                contentScaleX, contentScaleY);
            uploadEdgeQuadMesh(edgeQuadCpu, shapeEntries[g_activeShape].gpuEdges);

            //glClearColor(0.93f, 0.93f, 0.93f, 1.0f);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);

            glUseProgram(shaderProgram);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
            glUniformMatrix4fv(locView, 1, GL_FALSE, view.m);
            glUniformMatrix4fv(locProj, 1, GL_FALSE, proj.m);

            // Light direction expressed in VIEW space
            //  - because the view matrix changes every frame (camera orbits), transform the world-space light direction each frame
            //  - using hardcoded world-space direction (0.4, 0.7, 0.5), normalised inline
            const float lx = 0.4f, ly = 0.7f, lz = 0.5f;
            const float lLen = std::sqrt(lx * lx + ly * ly + lz * lz);
            // Multiply by the 3×3 rotation part of the view matrix (upper-left block)
            // view.m is column-major: column c, row r -> view.m[c*4 + r]
            const float vlx = view.m[0] * lx / lLen + view.m[4] * ly / lLen + view.m[8] * lz / lLen;
            const float vly = view.m[1] * lx / lLen + view.m[5] * ly / lLen + view.m[9] * lz / lLen;
            const float vlz = view.m[2] * lx / lLen + view.m[6] * ly / lLen + view.m[10] * lz / lLen;
            glUniform3f(locLightDir, vlx, vly, vlz);
            
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            drawGLMesh(shapeEntries[g_activeShape].gpuMesh);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(edgeShaderProgram);
            glUniformMatrix4fv(locEdgeViewProj, 1, GL_FALSE, edgeViewProj.m);
            glUniform4f(locEdgeColor, 1.0f, 0.645f, 0.0f, 1.0f);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            drawEdgeQuadMesh(shapeEntries[g_activeShape].gpuEdges);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            glDisable(GL_BLEND);

            // NEW: text output
            //  - NDC coordinates: x=-0.97 is near the left edge, y=0.96 is near the top
            drawText("Arrow keys - cycle shapes", -0.97f, 0.96f,
                1.0f,
                0.85f, 0.65f, 0.0f, 1.0f);

            drawText("V - toggle orthographic/perspective view", -0.97f, 0.92f,
                1.0f,
                0.85f, 0.65f, 0.0f, 1.0f);

            drawText("F11 - toggle fullscreen/window", -0.97f, 0.88f,
                1.0f,
                0.85f, 0.65f, 0.0f, 1.0f);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        for (ShapeEntry& entry : shapeEntries)
        {
            destroyEdgeQuadMesh(entry.gpuEdges);
            destroyGLMesh(entry.gpuMesh);
        }

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

        destroyFontRenderer();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';

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

        destroyFontRenderer();

        if (window != nullptr)
        {
            glfwDestroyWindow(window);
        }

        glfwTerminate();
        return -1;
    }
}