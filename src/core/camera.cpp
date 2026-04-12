#include "camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.1415926535f;

    float clampf(float v, float lo, float hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    Vec3 mul(const Vec3& v, float s)
    {
        return { v.x * s, v.y * s, v.z * s };
    }

    float lengthVec3(const Vec3& v)
    {
        return std::sqrt(dot(v, v));
    }

    Vec3 rotateAroundAxis(const Vec3& v, const Vec3& axisIn, float angleRad)
    {
        const Vec3 axis = normalize(axisIn);
        const float c   = std::cos(angleRad);
        const float s   = std::sin(angleRad);

        return {
            v.x * c + (axis.y * v.z - axis.z * v.y) * s + axis.x * dot(axis, v) * (1.0f - c),
            v.y * c + (axis.z * v.x - axis.x * v.z) * s + axis.y * dot(axis, v) * (1.0f - c),
            v.z * c + (axis.x * v.y - axis.y * v.x) * s + axis.z * dot(axis, v) * (1.0f - c)
        };
    }
}

Camera::Camera() = default;

void Camera::setViewport(int width, int height)
{
    m_viewWidth  = (width > 0) ? width : 1;
    m_viewHeight = (height > 0) ? height : 1;
}

void Camera::setPivot(const Vec3& pivot)
{
    m_pivot      = pivot;
}

Vec3 Camera::eye() const
{
    return eyePosition();
}

float Camera::distance() const
{
    return lengthVec3(m_offset);
}

void Camera::setDistance(float distanceValue)
{
    const float d = std::max(distanceValue, 0.01f);

    const float oldLen = lengthVec3(m_offset);
    if (oldLen <= 1.0e-12f)
    {
        m_offset  = { -d, 0.0f, 0.0f };
    }
    else
    {
        m_offset  = mul(normalize(m_offset), d);
    }
}

void Camera::setYawPitch(float yawRad, float pitchRad)
{
    const float pitch   = clampf(pitchRad, -1.55f, 1.55f);
    const float d       = distance();

    const float cp      = std::cos(pitch);
    const float sp      = std::sin(pitch);
    const float cy      = std::cos(yawRad);
    const float sy      = std::sin(yawRad);

    const Vec3 forward  = normalize({ cp * cy, cp * sy, sp });
    m_offset            = mul(forward, -d);

    const Vec3 worldUp{ 0.0f, 0.0f, 1.0f };
    Vec3 right          = cross(forward, worldUp);

    if (dot(right, right) <= 1.0e-12f)
    {
        right   = { 1.0f, 0.0f, 0.0f };
    }
    else
    {
        right   = normalize(right);
    }

    m_up = normalize(cross(right, forward));
}

Vec3 Camera::eyePosition() const
{
    return {
        m_pivot.x + m_offset.x,
        m_pivot.y + m_offset.y,
        m_pivot.z + m_offset.z
    };
}

Vec3 Camera::forwardDir() const
{
    return normalize(mul(m_offset, -1.0f));
}

Vec3 Camera::rightDir() const
{
    return normalize(cross(forwardDir(), m_up));
}

Vec3 Camera::upDir() const
{
    const Vec3 r = rightDir();
    const Vec3 f = forwardDir();
    return normalize(cross(r, f));
}

void Camera::orthonormalizeUp()
{
    const Vec3 f = forwardDir();
    Vec3 r       = cross(f, m_up);

    if (dot(r, r) <= 1.0e-12f)
    {
        r   = { 1.0f, 0.0f, 0.0f };
    }
    else
    {
        r   = normalize(r);
    }

    m_up = normalize(cross(r, f));
}

void Camera::beginRotate(double mouseX, double mouseY)
{
    m_rotating   = true;
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
}

void Camera::updateRotate(double mouseX, double mouseY)
{
    if (!m_rotating)
    {
        return;
    }

    const float dx  = static_cast<float>(mouseX - m_lastMouseX);
    const float dy  = static_cast<float>(mouseY - m_lastMouseY);

    m_lastMouseX    = mouseX;
    m_lastMouseY    = mouseY;

    const float yawSpeed    = 1.5f * kPi / static_cast<float>(m_viewWidth);
    const float pitchSpeed  = 0.75f * kPi / static_cast<float>(m_viewHeight);

    const float yawAngle    = -dx * yawSpeed;
    const float pitchAngle  = -dy * pitchSpeed;

    const Vec3 currentUp    = upDir();
    const Vec3 currentRight = rightDir();

    m_offset = rotateAroundAxis(m_offset, currentUp, yawAngle);
    m_up     = rotateAroundAxis(m_up, currentUp, yawAngle);

    m_offset = rotateAroundAxis(m_offset, currentRight, pitchAngle);
    m_up     = rotateAroundAxis(m_up, currentRight, pitchAngle);

    orthonormalizeUp();
}

void Camera::endRotate()
{
    m_rotating = false;
}

void Camera::beginPan(double mouseX, double mouseY)
{
    m_panning       = true;
    m_dragStartX    = mouseX;
    m_dragStartY    = mouseY;
    m_dragStartPanX = m_panViewX;
    m_dragStartPanY = m_panViewY;
}

void Camera::updatePan(double mouseX, double mouseY)
{
    if (!m_panning)
    {
        return;
    }

    const float dx      = static_cast<float>(mouseX - m_dragStartX);
    const float dy      = static_cast<float>(mouseY - m_dragStartY);

    const float aspect  = static_cast<float>(m_viewWidth) / static_cast<float>(m_viewHeight);
    const float halfW   = m_orthoHalfHeight * aspect;
    const float halfH   = m_orthoHalfHeight;

    const float worldPerPixelX = (2.0f * halfW) / static_cast<float>(m_viewWidth);
    const float worldPerPixelY = (2.0f * halfH) / static_cast<float>(m_viewHeight);

    // Pan is a VIEW-SPACE shift, not a second world target
    m_panViewX = m_dragStartPanX + dx * worldPerPixelX;
    m_panViewY = m_dragStartPanY - dy * worldPerPixelY;
}

void Camera::endPan()
{
    m_panning = false;
}

void Camera::zoomWheel(double mouseX, double mouseY, double wheelDelta)
{
    if (wheelDelta == 0.0)
    {
        return;
    }

    const float zoomBase    = 0.90f;
    const float factor      = std::pow(zoomBase, static_cast<float>(-wheelDelta));

    const float aspect      = static_cast<float>(m_viewWidth) / static_cast<float>(m_viewHeight);

    const float oldHalfH    = m_orthoHalfHeight;
    const float oldHalfW    = oldHalfH * aspect;

    float newHalfH  = oldHalfH * factor;
    newHalfH        = clampf(newHalfH, 0.001f, 100000.0f);

    const float newHalfW = newHalfH * aspect;

    // Zoom in follows cursor, zoom out stays fixed
    if (wheelDelta < 0.0)
    {
        // cursor in normalized device coordinates [-1, +1]
        const float nx = static_cast<float>((2.0 * mouseX / static_cast<double>(m_viewWidth)) - 1.0);
        const float ny = static_cast<float>(1.0 - (2.0 * mouseY / static_cast<double>(m_viewHeight)));

        // Adjust persistent view-space pan so the point under the cursor stays put
        m_panViewX += nx * (newHalfW - oldHalfW);
        m_panViewY += ny * (newHalfH - oldHalfH);
    }

    m_orthoHalfHeight = newHalfH;
}

Mat4 Camera::viewMatrix() const
{
    const Mat4 baseView = lookAt(eyePosition(), m_pivot, upDir());

    // Apply persistent screen-space pan after camera transform
    const Mat4 panView  = translation(m_panViewX, m_panViewY, 0.0f);

    return multiply(panView, baseView);
}

Mat4 Camera::projMatrix() const
{
    const float aspect = static_cast<float>(m_viewWidth) / static_cast<float>(m_viewHeight);

    if (m_orthographic)
    {
        // Existing orthographic projection
        const float halfW = m_orthoHalfHeight * aspect;
        const float halfH = m_orthoHalfHeight;

        return orthographic(
            -halfW, halfW,
            -halfH, halfH,
            m_nearPlane, m_farPlane
        );
    }
    else
    {
        // New: perspective projection
        // Derive matching FOV from current ortho half-height and camera distance so subject stays _roughly_ same size when toggling
        const float dist = distance();
        const float fovy = 2.0f * std::atan2(m_orthoHalfHeight, dist);

        return perspective(fovy, aspect, m_nearPlane, m_farPlane);
    }
}

void Camera::toggleProjection()
{
    m_orthographic = !m_orthographic;
}
