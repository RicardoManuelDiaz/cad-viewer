#pragma once

#include "math3d.h"

class Camera
{
public:
    Camera();

    void setViewport(int width, int height);
    void setPivot(const Vec3& pivot);
    void setDistance(float distanceValue);
    void setYawPitch(float yawRad, float pitchRad);

    void beginRotate(double mouseX, double mouseY);
    void updateRotate(double mouseX, double mouseY);
    void endRotate();

    void beginPan(double mouseX, double mouseY);
    void updatePan(double mouseX, double mouseY);
    void endPan();

    void zoomWheel(double mouseX, double mouseY, double wheelDelta);

    Mat4 viewMatrix() const;
    Mat4 projMatrix() const;

    const Vec3& pivot() const { return m_pivot; }
    Vec3 target() const { return m_pivot; } // force same logical center
    Vec3 eye() const;

    float distance() const;

private:
    Vec3 eyePosition() const;
    Vec3 forwardDir() const;
    Vec3 rightDir() const;
    Vec3 upDir() const;

    void orthonormalizeUp();

private:
    int   m_viewWidth{ 1 };
    int   m_viewHeight{ 1 };

    // Real orbit center in world space
    Vec3  m_pivot{ 0.0f, 0.0f, 0.0f };

    // eye = pivot + offset
    Vec3  m_offset{ -40.0f, -40.0f, 30.0f };
    Vec3  m_up{ 0.0f,   0.0f,  1.0f };

    // Persistent screen-space pan, stored in view-space world units
    float m_panViewX{ 0.0f };
    float m_panViewY{ 0.0f };

    float m_orthoHalfHeight{ 20.0f };
    float m_nearPlane{ 0.1f };
    float m_farPlane{ 500.0f };

    bool   m_rotating{ false };
    bool   m_panning{ false };

    double m_lastMouseX{ 0.0 };
    double m_lastMouseY{ 0.0 };

    double m_dragStartX{ 0.0 };
    double m_dragStartY{ 0.0 };

    float  m_dragStartPanX{ 0.0f };
    float  m_dragStartPanY{ 0.0f };
};