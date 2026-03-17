#include "math3d.h"

#include <cmath>

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 normalize(const Vec3& v)
{
    const float lenSq = dot(v, v);
    if (lenSq <= 1.0e-20f)
    {
        return { 0.0f, 0.0f, 0.0f };
    }

    const float invLen = 1.0f / std::sqrt(lenSq);
    return { v.x * invLen, v.y * invLen, v.z * invLen };
}

Mat4 identity()
{
    Mat4 out{};
    out.m[0]        = 1.0f;
    out.m[5]        = 1.0f;
    out.m[10]       = 1.0f;
    out.m[15]       = 1.0f;
    return out;
}

Mat4 multiply(const Mat4& a, const Mat4& b)
{
    Mat4 out{};

    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            out.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }

    return out;
}

Mat4 translation(float tx, float ty, float tz)
{
    Mat4 out        = identity();
    out.m[12]       = tx;
    out.m[13]       = ty;
    out.m[14]       = tz;
    return out;
}

Mat4 rotationZ(float angleRad)
{
    Mat4 out        = identity();

    const float c   = std::cos(angleRad);
    const float s   = std::sin(angleRad);

    out.m[0]        = c;
    out.m[4]        = -s;
    out.m[1]        = s;
    out.m[5]        = c;

    return out;
}

Mat4 perspective(float fovyRad, float aspect, float zNear, float zFar)
{
    Mat4 out{};

    const float tanHalf = std::tan(fovyRad * 0.5f);

    out.m[0]        = 1.0f / (aspect * tanHalf);
    out.m[5]        = 1.0f / tanHalf;
    out.m[10]       = -(zFar + zNear) / (zFar - zNear);
    out.m[11]       = -1.0f;
    out.m[14]       = -(2.0f * zFar * zNear) / (zFar - zNear);

    return out;
}

Mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4 out{};

    out.m[0]        = 2.0f / (right - left);
    out.m[5]        = 2.0f / (top - bottom);
    out.m[10]       = -2.0f / (zFar - zNear);
    out.m[12]       = -(right + left) / (right - left);
    out.m[13]       = -(top + bottom) / (top - bottom);
    out.m[14]       = -(zFar + zNear) / (zFar - zNear);
    out.m[15]       = 1.0f;

    return out;
}

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    const Vec3 f    = normalize(center - eye);
    const Vec3 s    = normalize(cross(f, up));
    const Vec3 u    = cross(s, f);

    Mat4 out        = identity();

    out.m[0]        = s.x;
    out.m[4]        = s.y;
    out.m[8]        = s.z;

    out.m[1]        = u.x;
    out.m[5]        = u.y;
    out.m[9]        = u.z;

    out.m[2]        = -f.x;
    out.m[6]        = -f.y;
    out.m[10]       = -f.z;

    out.m[12]       = -dot(s, eye);
    out.m[13]       = -dot(u, eye);
    out.m[14]       = dot(f, eye);

    return out;
}