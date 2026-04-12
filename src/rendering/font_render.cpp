#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "font_render.h"
#include "shader_utils.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <stdexcept>

// -------------------------------------------------------------------
// Atlas parameters
// -------------------------------------------------------------------
static constexpr int   kAtlasW = 512;
static constexpr int   kAtlasH = 512;
static constexpr int   kFirstChar = 32;    // space
static constexpr int   kNumChars = 96;    // through ~

// -------------------------------------------------------------------
// State
// -------------------------------------------------------------------
static GLuint               g_atlasTexture = 0;
static GLuint               g_vao = 0;
static GLuint               g_vbo = 0;
static GLuint               g_prog = 0;
static stbtt_bakedchar      g_charData[kNumChars]{};
static float                g_bakePixelH = 18.0f;
static int                  g_vpW = 1;
static int                  g_vpH = 1;

// -------------------------------------------------------------------
// Shaders — simple textured quad, alpha from red channel
// -------------------------------------------------------------------
static const char* kFontVS = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main()
{
    vUV         = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* kFontFS = R"(
#version 330 core
in  vec2      vUV;
out vec4      FragColor;
uniform sampler2D uAtlas;
uniform vec4      uColor;
void main()
{
    float alpha = texture(uAtlas, vUV).r;
    FragColor   = vec4(uColor.rgb, uColor.a * alpha);
}
)";

// -------------------------------------------------------------------
bool initFontRenderer(const std::string& fontPath, float fontPx)
{
    g_bakePixelH = fontPx;

    // Load TTF bytes
    FILE* f = nullptr;
    if (fopen_s(&f, fontPath.c_str(), "rb") != 0 || !f)
        return false;

    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<unsigned char> ttfBuf(static_cast<std::size_t>(sz));
    fread(ttfBuf.data(), 1, static_cast<std::size_t>(sz), f);
    fclose(f);

    // Bake glyphs into a greyscale atlas
    std::vector<unsigned char> atlasBuf(kAtlasW * kAtlasH, 0);

    const int result = stbtt_BakeFontBitmap(
        ttfBuf.data(), 0,
        fontPx,
        atlasBuf.data(), kAtlasW, kAtlasH,
        kFirstChar, kNumChars,
        g_charData);

    if (result <= 0)
        return false;   // not all glyphs fit — increase atlas size if needed

    // Upload atlas as a single-channel (RED) texture
    glGenTextures(1, &g_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, g_atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
        kAtlasW, kAtlasH, 0,
        GL_RED, GL_UNSIGNED_BYTE, atlasBuf.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // VAO / VBO for streaming quads (6 verts * (2+2) floats per character)
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);

    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    glBindVertexArray(0);

    // Shader
    g_prog = createShaderProgram(kFontVS, kFontFS);

    return true;
}

// -------------------------------------------------------------------
void destroyFontRenderer()
{
    if (g_atlasTexture) { glDeleteTextures(1, &g_atlasTexture); g_atlasTexture = 0; }
    if (g_vbo) { glDeleteBuffers(1, &g_vbo);           g_vbo = 0; }
    if (g_vao) { glDeleteVertexArrays(1, &g_vao);      g_vao = 0; }
    if (g_prog) { glDeleteProgram(g_prog);               g_prog = 0; }
}

// -------------------------------------------------------------------
void setFontViewport(int width, int height)
{
    g_vpW = (width > 0) ? width : 1;
    g_vpH = (height > 0) ? height : 1;
}

// -------------------------------------------------------------------
void drawText(const std::string& text,
    float x, float y,
    float scale,
    float r, float g, float b, float a)
{
    if (!g_prog || !g_atlasTexture || text.empty())
        return;

    // Convert the NDC origin to pixel space for stb to work in,
    // then convert each quad back to NDC for the shader.
    // stb works in screen pixels (Y down), NDC is Y up.
    // x,y are NDC top-left of the first glyph.
    //
    // NDC -> pixel:  px = (ndcX + 1) * 0.5 * vpW
    //                py = (1 - ndcY) * 0.5 * vpH   (flip Y)

    float cursorPxX = (x + 1.0f) * 0.5f * static_cast<float>(g_vpW);
    float cursorPxY = (1.0f - y) * 0.5f * static_cast<float>(g_vpH);

    // Each character = 2 triangles = 6 vertices, each vertex = (x,y,u,v)
    std::vector<float> verts;
    verts.reserve(text.size() * 6 * 4);

    auto pxToNdcX = [&](float px) { return  px / static_cast<float>(g_vpW) * 2.0f - 1.0f; };
    auto pxToNdcY = [&](float py) { return -py / static_cast<float>(g_vpH) * 2.0f + 1.0f; };

    for (const char ch : text)
    {
        const int codepoint = static_cast<unsigned char>(ch);
        if (codepoint < kFirstChar || codepoint >= kFirstChar + kNumChars)
        {
            // Space or unknown — just advance
            cursorPxX += g_bakePixelH * 0.3f * scale;
            continue;
        }

        stbtt_aligned_quad q{};
        stbtt_GetBakedQuad(
            g_charData,
            kAtlasW, kAtlasH,
            codepoint - kFirstChar,
            &cursorPxX, &cursorPxY,
            &q,
            1);   // 1 = opengl UV convention (Y=0 at bottom of atlas)

        // Scale around the cursor start if scale != 1
        if (scale != 1.0f)
        {
            // q gives positions relative to cursor — scale them
            const float cx = pxToNdcX(cursorPxX);
            const float cy = pxToNdcY(cursorPxY);
            (void)cx; (void)cy;
        }

        // Two triangles (CCW)
        // Bottom-left, bottom-right, top-right, bottom-left, top-right, top-left
        //   stb quad:  x0,y0 = top-left   x1,y1 = bottom-right  (pixel Y down)

        const float ndcX0 = pxToNdcX(q.x0), ndcX1 = pxToNdcX(q.x1);
        const float ndcY0 = pxToNdcY(q.y0), ndcY1 = pxToNdcY(q.y1);

        // Triangle 1
        verts.insert(verts.end(), { ndcX0, ndcY1,  q.s0, q.t1 });  // top-left
        verts.insert(verts.end(), { ndcX1, ndcY1,  q.s1, q.t1 });  // top-right
        verts.insert(verts.end(), { ndcX1, ndcY0,  q.s1, q.t0 });  // bottom-right
        // Triangle 2
        verts.insert(verts.end(), { ndcX0, ndcY1,  q.s0, q.t1 });  // top-left
        verts.insert(verts.end(), { ndcX1, ndcY0,  q.s1, q.t0 });  // bottom-right
        verts.insert(verts.end(), { ndcX0, ndcY0,  q.s0, q.t0 });  // bottom-left
    }

    if (verts.empty())
        return;

    // Upload and draw
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
        verts.data(),
        GL_DYNAMIC_DRAW);

    glUseProgram(g_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_atlasTexture);
    glUniform1i(glGetUniformLocation(g_prog, "uAtlas"), 0);
    glUniform4f(glGetUniformLocation(g_prog, "uColor"), r, g, b, a);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(g_vao);
    glDrawArrays(GL_TRIANGLES, 0,
        static_cast<GLsizei>(verts.size() / 4));
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
