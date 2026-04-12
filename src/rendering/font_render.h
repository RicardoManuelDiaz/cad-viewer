#pragma once

#include <glad/glad.h>
#include <string>

// Initialise the font renderer
// fontPath : path to a .ttf file
// fontPx   : glyph height in pixels for the atlas
// Returns true on success
bool initFontRenderer(const std::string& fontPath, float fontPx = 18.0f);

// Release all GPU resources
void destroyFontRenderer();

// Draw a string in normalised screen coordinates
// x, y     : top-left origin in NDC [-1, +1], Y up
// scale    : multiplier on the baked font size
// r,g,b,a  : colour
void drawText(const std::string& text,
    float x, float y,
    float scale = 1.0f,
    float r = 0.0f, float g = 0.0f,
    float b = 0.0f, float a = 1.0f);

// Call once per frame before any drawText calls — sets the viewport dimensions used to convert pixels to NDC
void setFontViewport(int width, int height);
