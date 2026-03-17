#pragma once

struct Vec3
{
    float x{};
    float y{};
    float z{};
};

struct Mat4
{
    float m[16]{};
};

Vec3 operator-(const Vec3& a, const Vec3& b);

Vec3 cross(const Vec3& a, const Vec3& b);
float dot(const Vec3& a, const Vec3& b);
Vec3 normalize(const Vec3& v);

Mat4 identity();
Mat4 multiply(const Mat4& a, const Mat4& b);
Mat4 translation(float tx, float ty, float tz);
Mat4 rotationZ(float angleRad);
Mat4 perspective(float fovyRad, float aspect, float zNear, float zFar);
Mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);