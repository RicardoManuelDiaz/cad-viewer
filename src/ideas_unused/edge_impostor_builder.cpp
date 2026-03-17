#include "edge_impostor_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

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

    struct ProjPoint
    {
        Vec4f clip;
        Vec2f px;
        Vec2f ndc;
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

    float lengthSq(const Vec2f& v)
    {
        return dot2(v, v);
    }

    float length(const Vec2f& v)
    {
        return std::sqrt(lengthSq(v));
    }

    Vec2f normalizeSafe(const Vec2f& v)
    {
        const float lsq = lengthSq(v);
        if (lsq <= 1.0e-12f)
            return { 0.0f, 0.0f };

        return v / std::sqrt(lsq);
    }

    Vec2f perp(const Vec2f& v)
    {
        return { -v.y, v.x };
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

    ProjPoint projectPoint(const Mat4& viewProj, const Vec3& p, const Viewport& vp)
    {
        ProjPoint out{};
        out.clip = mulPoint(viewProj, p);

        const float invW = 1.0f / out.clip.w;
        out.ndc = { out.clip.x * invW, out.clip.y * invW };

        out.px = {
            (out.ndc.x * 0.5f + 0.5f) * static_cast<float>(vp.width),
            (out.ndc.y * 0.5f + 0.5f) * static_cast<float>(vp.height)
        };

        return out;
    }

    Vec2f pixelOffsetToNdc(const Vec2f& px, const Viewport& vp)
    {
        return {
            (2.0f * px.x) / static_cast<float>(vp.width),
            (2.0f * px.y) / static_cast<float>(vp.height)
        };
    }

    Vec4f offsetClipXY(const Vec4f& clip, const Vec2f& offsetNdc, float depthBiasNdc)
    {
        Vec4f out = clip;
        out.x += offsetNdc.x * clip.w;
        out.y += offsetNdc.y * clip.w;
        out.z -= depthBiasNdc * clip.w;
        return out;
    }

    void rotateClosedLoopByOffset(std::vector<ProjPoint>& pts, std::size_t offset)
    {
        if (pts.empty())
            return;

        offset %= pts.size();
        if (offset == 0)
            return;

        std::rotate(pts.begin(), pts.begin() + static_cast<std::ptrdiff_t>(offset), pts.end());
    }

    std::size_t chooseClosedLoopSeamIndex_BackArc(const std::vector<ProjPoint>& pts)
    {
        if (pts.empty())
            return 0;

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

    Vec2f computeStartNormal(
        const std::vector<ProjPoint>& pts,
        int i0,
        int i1,
        bool closed)
    {
        const int n = static_cast<int>(pts.size());
        const Vec2f a = pts[i0].px;
        const Vec2f b = pts[i1].px;
        const Vec2f d = normalizeSafe(b - a);

        if (!closed && i0 == 0)
            return d;

        const int ip = (i0 == 0) ? (n - 1) : (i0 - 1);
        const Vec2f p = pts[ip].px;
        const Vec2f dPrev = normalizeSafe(a - p);

        Vec2f nStart = normalizeSafe(dPrev + d);
        if (lengthSq(nStart) <= 1.0e-12f)
            nStart = d;

        return nStart;
    }

    Vec2f computeEndNormal(
        const std::vector<ProjPoint>& pts,
        int i0,
        int i1,
        bool closed)
    {
        const int n = static_cast<int>(pts.size());
        const Vec2f a = pts[i0].px;
        const Vec2f b = pts[i1].px;
        const Vec2f d = normalizeSafe(b - a);

        if (!closed && i1 == n - 1)
            return -d;

        const int in = (i1 == n - 1) ? 0 : (i1 + 1);
        const Vec2f pNext = pts[in].px;
        const Vec2f dNext = normalizeSafe(pNext - b);

        Vec2f nEnd = -normalizeSafe(d + dNext);
        if (lengthSq(nEnd) <= 1.0e-12f)
            nEnd = -d;

        return nEnd;
    }

    float computeStartExtensionPx(const Vec2f& segDir, const Vec2f& startN, float halfWidthPx, float miterLimitPx)
    {
        const float denom = std::max(dot2(startN, segDir), 1.0e-4f);
        return std::min(halfWidthPx / denom, halfWidthPx * miterLimitPx);
    }

    float computeEndExtensionPx(const Vec2f& segDir, const Vec2f& endN, float halfWidthPx, float miterLimitPx)
    {
        const float denom = std::max(dot2(-endN, segDir), 1.0e-4f);
        return std::min(halfWidthPx / denom, halfWidthPx * miterLimitPx);
    }

    EdgeImpostorVertex makeVertex(
        const Vec4f& clip,
        const Vec2f& aPx,
        const Vec2f& bPx,
        const Vec2f& startN,
        const Vec2f& endN,
        float halfWidthPx,
        std::uint32_t ownerEdgeId)
    {
        return EdgeImpostorVertex{
            clip.x, clip.y, clip.z, clip.w,

            aPx.x, bPx.x ? aPx.y : aPx.y, // placeholder fixed below by aggregate order
            bPx.x, bPx.y,

            startN.x, startN.y,
            endN.x, endN.y,

            halfWidthPx,
            ownerEdgeId
        };
    }

    EdgeImpostorVertex makeVertexFixed(
        const Vec4f& clip,
        const Vec2f& aPx,
        const Vec2f& bPx,
        const Vec2f& startN,
        const Vec2f& endN,
        float halfWidthPx,
        std::uint32_t ownerEdgeId)
    {
        return EdgeImpostorVertex{
            clip.x, clip.y, clip.z, clip.w,
            aPx.x, aPx.y,
            bPx.x, bPx.y,
            startN.x, startN.y,
            endN.x, endN.y,
            halfWidthPx,
            ownerEdgeId
        };
    }
}

EdgeImpostorMeshCpu buildEdgeImpostorMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    const Viewport& viewport,
    float widthPx,
    float miterLimitPx,
    float depthBiasNdc)
{
    EdgeImpostorMeshCpu out;

    if (viewport.width <= 0 || viewport.height <= 0)
        return out;

    const float halfWidthPx = 0.5f * widthPx;

    for (const EdgePolyline3D& polyline : polylines)
    {
        if (polyline.points.size() < 2)
            continue;

        std::vector<ProjPoint> pts;
        pts.reserve(polyline.points.size());

        for (const Vec3& p : polyline.points)
            pts.push_back(projectPoint(viewProj, p, viewport));

        if (polyline.closed && pts.size() >= 4)
        {
            const std::size_t seamIndex = chooseClosedLoopSeamIndex_BackArc(pts);
            rotateClosedLoopByOffset(pts, seamIndex);
        }

        const int pointCount = static_cast<int>(pts.size());
        const int segCount = polyline.closed ? pointCount : (pointCount - 1);

        for (int i = 0; i < segCount; ++i)
        {
            const int j = (i + 1) % pointCount;

            const Vec2f aPx = pts[i].px;
            const Vec2f bPx = pts[j].px;

            const Vec2f segDir = normalizeSafe(bPx - aPx);
            if (lengthSq(segDir) <= 1.0e-12f)
                continue;

            const Vec2f segPerp = perp(segDir);

            const Vec2f startN = computeStartNormal(pts, i, j, polyline.closed);
            const Vec2f endN = computeEndNormal(pts, i, j, polyline.closed);

            const float startExtPx = computeStartExtensionPx(segDir, startN, halfWidthPx, miterLimitPx);
            const float endExtPx = computeEndExtensionPx(segDir, endN, halfWidthPx, miterLimitPx);

            const Vec2f aExtPx = aPx - segDir * startExtPx;
            const Vec2f bExtPx = bPx + segDir * endExtPx;

            const Vec2f sidePx = segPerp * halfWidthPx;
            const Vec2f sideNdc = pixelOffsetToNdc(sidePx, viewport);

            const Vec2f startShiftPx = -segDir * startExtPx;
            const Vec2f endShiftPx = segDir * endExtPx;

            const Vec2f startShiftNdc = pixelOffsetToNdc(startShiftPx, viewport);
            const Vec2f endShiftNdc = pixelOffsetToNdc(endShiftPx, viewport);

            const Vec4f clipA = offsetClipXY(pts[i].clip, startShiftNdc, depthBiasNdc);
            const Vec4f clipB = offsetClipXY(pts[j].clip, endShiftNdc, depthBiasNdc);

            const Vec4f q0 = offsetClipXY(clipA, sideNdc, depthBiasNdc);
            const Vec4f q1 = offsetClipXY(clipA, -sideNdc, depthBiasNdc);
            const Vec4f q2 = offsetClipXY(clipB, sideNdc, depthBiasNdc);
            const Vec4f q3 = offsetClipXY(clipB, -sideNdc, depthBiasNdc);

            const std::uint32_t base = static_cast<std::uint32_t>(out.vertices.size());
            const std::uint32_t owner = static_cast<std::uint32_t>(polyline.ownerEdgeId);

            out.vertices.push_back(makeVertexFixed(q0, aPx, bPx, startN, endN, halfWidthPx, owner));
            out.vertices.push_back(makeVertexFixed(q1, aPx, bPx, startN, endN, halfWidthPx, owner));
            out.vertices.push_back(makeVertexFixed(q2, aPx, bPx, startN, endN, halfWidthPx, owner));
            out.vertices.push_back(makeVertexFixed(q3, aPx, bPx, startN, endN, halfWidthPx, owner));

            out.indices.push_back(base + 0);
            out.indices.push_back(base + 2);
            out.indices.push_back(base + 1);

            out.indices.push_back(base + 1);
            out.indices.push_back(base + 2);
            out.indices.push_back(base + 3);
        }
    }

    return out;
}