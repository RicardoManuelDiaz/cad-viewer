#include "edge_quad_builder.h"

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace
{
    struct Vec2f
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Vec4f
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
    };

    Vec2f operator+(const Vec2f& a, const Vec2f& b)
    {
        return { a.x + b.x, a.y + b.y };
    }

    Vec2f operator-(const Vec2f& a, const Vec2f& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    Vec2f operator-(const Vec2f& v)
    {
        return { -v.x, -v.y };
    }

    Vec2f operator*(const Vec2f& v, float s)
    {
        return { v.x * s, v.y * s };
    }

    Vec2f operator/(const Vec2f& v, float s)
    {
        return { v.x / s, v.y / s };
    }

    float dot2(const Vec2f& a, const Vec2f& b)
    {
        return a.x * b.x + a.y * b.y;
    }

    struct ClipPoint
    {
        Vec4f clip;
        Vec2f ndc;
        Vec2f px;
    };

    Vec2f perp(const Vec2f& v)
    {
        return Vec2f{ -v.y, v.x };
    }

    float lengthSq(const Vec2f& v)
    {
        return dot2(v, v);
    }

    Vec2f normalizeSafe(const Vec2f& v)
    {
        const float lsq = lengthSq(v);
        if (lsq <= 1e-12f)
            return Vec2f{ 0.0f, 0.0f };

        return v / std::sqrt(lsq);
    }

    Vec4f mulPoint(const Mat4& m, const Vec3& p)
    {
        return Vec4f{
            m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
            m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
            m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
            m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15]
        };
    }

    ClipPoint projectPoint(const Mat4& viewProj, const Vec3& p, const Viewport& vp)
    {
        ClipPoint out{};
        out.clip = mulPoint(viewProj, p);

        const float invW = 1.0f / out.clip.w;
        out.ndc = Vec2f{ out.clip.x * invW, out.clip.y * invW };

        out.px = Vec2f{
            (out.ndc.x * 0.5f + 0.5f) * static_cast<float>(vp.width),
            (out.ndc.y * 0.5f + 0.5f) * static_cast<float>(vp.height)
        };

        return out;
    }

    Vec2f pxToNdcOffset(const Vec2f& px, const Viewport& vp)
    {
        return Vec2f{
            (2.0f * px.x) / static_cast<float>(vp.width),
            (2.0f * px.y) / static_cast<float>(vp.height)
        };
    }

    Vec2f computeVertexOffsetPx(
        const std::vector<ClipPoint>& pts,
        int i,
        bool closed,
        float halfWidthPx,
        const EdgeStyle& style)
    {
        const int n = static_cast<int>(pts.size());

        if (n < 2)
            return Vec2f{ 0.0f, 0.0f };

        if (!closed)
        {
            if (i == 0)
            {
                const Vec2f d = normalizeSafe(pts[1].px - pts[0].px);
                return perp(d) * halfWidthPx;
            }

            if (i == n - 1)
            {
                const Vec2f d = normalizeSafe(pts[n - 1].px - pts[n - 2].px);
                return perp(d) * halfWidthPx;
            }
        }

        const int iPrev = (i == 0) ? (closed ? n - 1 : 0) : (i - 1);
        const int iNext = (i == n - 1) ? (closed ? 0 : n - 1) : (i + 1);

        const Vec2f d0  = normalizeSafe(pts[i].px - pts[iPrev].px);
        const Vec2f d1  = normalizeSafe(pts[iNext].px - pts[i].px);

        const Vec2f n0  = perp(d0);
        const Vec2f n1  = perp(d1);

        if (style.join == LineJoin::Bevel)
            return n1 * halfWidthPx;

        Vec2f m = n0 + n1;
        if (lengthSq(m) <= 1e-10f)
            return n1 * halfWidthPx;

        m = normalizeSafe(m);

        const float denom = dot2(m, n1);
        if (std::fabs(denom) <= 1e-5f)
            return n1 * halfWidthPx;

        const float miterLen = halfWidthPx / denom;
        if (std::fabs(miterLen) > style.miterLimit * halfWidthPx)
            return n1 * halfWidthPx;

        return m * miterLen;
    }

    EdgeQuadVertex makeVertex(const Vec4f& clip, std::uint32_t ownerEdgeId)
    {
        return EdgeQuadVertex{
            clip.x, clip.y, clip.z, clip.w,
            ownerEdgeId
        };
    }

    Vec4f offsetClipXY(const Vec4f& clip, const Vec2f& offsetNdc /*float depthBiasNdc*/)
    {
        Vec4f out = clip;
        out.x += offsetNdc.x * clip.w;
        out.y += offsetNdc.y * clip.w;
        //out.z -= depthBiasNdc * clip.w;
        return out;
    }

    //
    static void rotateClosedLoopByOffset(std::vector<ClipPoint>& pts, std::size_t offset)
    {
        if (pts.empty())
            return;

        offset %= pts.size();
        if (offset == 0)
            return;

        std::rotate(pts.begin(), pts.begin() + static_cast<std::ptrdiff_t>(offset), pts.end());
    }

    static std::size_t chooseClosedLoopSeamIndex_BackArc(const std::vector<ClipPoint>& pts)
    {
        if (pts.empty())
            return 0;

        // Simple heuristic:
        // put the seam at the point with the largest NDC Y,
        // which usually places it on the back/top arc for a cylinder top ellipse.
        std::size_t bestIndex = 0;
        float bestY = pts[0].ndc.y;

        for (std::size_t i = 1; i < pts.size(); ++i)
        {
            if (pts[i].ndc.y > bestY)
            {
                bestY = pts[i].ndc.y;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    void appendPolyline(
        const EdgePolyline3D& polyline,
        const Mat4& viewProj,
        const Viewport& viewport,
        const EdgeStyle& style,
        EdgeQuadMeshCpu& out)
    {
        if (polyline.points.size() < 2)
            return;

        std::vector<ClipPoint> pts;
        pts.reserve(polyline.points.size());

        for (const Vec3& p : polyline.points)
            pts.push_back(projectPoint(viewProj, p, viewport));

        if (polyline.closed && pts.size() >= 4)
        {
            const std::size_t seamIndex = chooseClosedLoopSeamIndex_BackArc(pts);
            rotateClosedLoopByOffset(pts, seamIndex);
        }

        const std::uint32_t base =
            static_cast<std::uint32_t>(out.vertices.size());

        const float halfWidthPx = 0.5f * style.widthPx;

        for (int i = 0; i < static_cast<int>(pts.size()); ++i)
        {
            const Vec2f offsetPx =
                computeVertexOffsetPx(pts, i, polyline.closed, halfWidthPx, style);

            const Vec2f offsetNdc = pxToNdcOffset(offsetPx, viewport);

            const Vec4f leftClip =
                offsetClipXY(pts[i].clip, offsetNdc /*style.depthBiasNdc*/);

            const Vec4f rightClip =
                offsetClipXY(pts[i].clip, -offsetNdc /*style.depthBiasNdc*/);

            out.vertices.push_back(
                makeVertex(leftClip, static_cast<std::uint32_t>(polyline.ownerEdgeId)));
            out.vertices.push_back(
                makeVertex(rightClip, static_cast<std::uint32_t>(polyline.ownerEdgeId)));
        }

        const std::uint32_t pointCount = static_cast<std::uint32_t>(pts.size());
        const std::uint32_t segCount = polyline.closed ? pointCount : (pointCount - 1);

        for (std::uint32_t i = 0; i < segCount; ++i)
        {
            const std::uint32_t j = (i + 1) % pointCount;

            const std::uint32_t i0 = base + 2 * i;
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = base + 2 * j;
            const std::uint32_t i3 = i2 + 1;

            out.indices.push_back(i0);
            out.indices.push_back(i2);
            out.indices.push_back(i1);

            out.indices.push_back(i1);
            out.indices.push_back(i2);
            out.indices.push_back(i3);
        }
    }
}

EdgeQuadMeshCpu buildEdgeQuadMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    const Viewport& viewport,
    const EdgeStyle& style)
{
    EdgeQuadMeshCpu out;

    if (viewport.width <= 0 || viewport.height <= 0)
        return out;

    for (const EdgePolyline3D& p : polylines)
        appendPolyline(p, viewProj, viewport, style, out);

    return out;
}