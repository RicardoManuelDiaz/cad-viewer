#include "edge_quad_builder.h"

#include <cmath>
#include <cstdint>

namespace
{
    // Minimal Vec2 helpers — avoids adding Vec2 to math3d
    //  - might want to actally add a Vec2 though at some point...
    struct V2 { float x, y; };

    static V2    v2sub(V2 a, V2 b)      { return { a.x - b.x, a.y - b.y }; }
    static V2    v2add(V2 a, V2 b)      { return { a.x + b.x, a.y + b.y }; }
    static V2    v2scale(V2 a, float s) { return { a.x * s, a.y * s }; }
    static float v2dot(V2 a, V2 b)      { return a.x * b.x + a.y * b.y; }
    static float v2len(V2 a)            { return std::sqrt(a.x * a.x + a.y * a.y); }
    static V2    v2norm(V2 a)
    {
        const float l = v2len(a);
        return (l > 1e-12f) ? V2{ a.x / l, a.y / l } : V2{ 0,0 };
    }

    // Perpendicular (left normal): rotates 90° CCW
    static V2    v2perp(V2 a)           { return { -a.y, a.x }; }

    // -------------------------------------------------------
    // Project one world point->clip space (x,y,z,w)
    // -------------------------------------------------------
    struct ClipPt { float x, y, z, w; };

    static ClipPt projectPoint(const Mat4& m, const Vec3& p)
    {
        // Column-major multiply
        return {
            m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
            m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
            m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
            m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15]
        };
    }

    // NDC xy from clip (needed for screen-space direction vectors)
    static V2 ndcXY(ClipPt c)
    {
        if (std::abs(c.w) < 1e-12f) return { 0,0 };
        return { c.x / c.w, c.y / c.w };
    }

    // -------------------------------------------------------
    // Emit one quad (two triangles) for a segment
    //  - p0, p1 are clip-space endpoints
    //  - off0, off1 are the NDC-space perpendicular half-width
    //  - offsets at each end, already scaled back to clip space.
    // -------------------------------------------------------
    static void emitSegmentQuad(
        EdgeQuadMeshCpu& out,
        const ClipPt& p0, const V2& off0,
        const ClipPt& p1, const V2& off1,
        std::uint32_t edgeId)
    {
        //  3---2
        //  |  /|
        //  | / |
        //  |/  |
        //  0---1
        //
        //  0 = p0 - off0
        //  1 = p0 + off0
        //  2 = p1 + off1
        //  3 = p1 - off1

        const std::uint32_t base = static_cast<std::uint32_t>(out.vertices.size());

        auto addV = [&](const ClipPt& p, V2 o, float cov)
            {
                out.vertices.push_back({
                    p.x + o.x * p.w,
                    p.y + o.y * p.w,
                    p.z,
                    p.w,
                    cov,
                    edgeId
                    });
            };

        addV(p0, { -off0.x, -off0.y }, -1.0f);  // 0  — left edge
        addV(p0, {  off0.x,  off0.y }, +1.0f);  // 1  — right edge
        addV(p1, {  off1.x,  off1.y }, +1.0f);  // 2  — right edge
        addV(p1, { -off1.x, -off1.y }, -1.0f);  // 3  — left edge

        // Two triangles: 0-1-2 and 0-2-3
        out.indices.push_back(base + 0);
        out.indices.push_back(base + 1);
        out.indices.push_back(base + 2);
        out.indices.push_back(base + 0);
        out.indices.push_back(base + 2);
        out.indices.push_back(base + 3);
    }

    // -------------------------------------------------------
    // Emit a bevel triangle to fill the gap at a join
    //  - pivot is the shared endpoint; a and b are the two outer corners from the adjacent segments
    // -------------------------------------------------------
    static void emitBevelTriangle(
        EdgeQuadMeshCpu& out,
        const ClipPt& pivot,
        const V2& a,     // NDC offset for corner from seg i
        const V2& b,     // NDC offset for corner from seg i+1
        std::uint32_t edgeId)
    {
        const std::uint32_t base = static_cast<std::uint32_t>(out.vertices.size());

        auto addV = [&](const ClipPt& p, V2 o, float cov)
            {
                out.vertices.push_back({
                    p.x + o.x * p.w,
                    p.y + o.y * p.w,
                    p.z,
                    p.w,
                    cov,
                    edgeId
                    });
            };

        // The pivot is at the centre, the two outer corners are at the edge, so...
        addV(pivot, {0, 0},  0.0f);   // centre
        addV(pivot, a,      +1.0f);   // outer corner
        addV(pivot, b,      +1.0f);   // outer corner

        out.indices.push_back(base + 0);
        out.indices.push_back(base + 1);
        out.indices.push_back(base + 2);
    }

    // -------------------------------------------------------
    // Expand one polyline into quads
    // -------------------------------------------------------
    static void expandPolyline(
        const EdgePolyline3D& poly,
        const Mat4& viewProj,
        float halfWidthNdcX,   // half-width in NDC x
        float halfWidthNdcY,   // half-width in NDC y
        const EdgeStyle& style,
        EdgeQuadMeshCpu& out)
    {
        const int n = static_cast<int>(poly.points.size());
        if (n < 2) return;

        // Project all points to clip space upfront
        std::vector<ClipPt> clip(n);
        for (int i = 0; i < n; ++i)
            clip[i] = projectPoint(viewProj, poly.points[i]);

        const std::uint32_t edgeId =
            static_cast<std::uint32_t>(poly.ownerEdgeId);

        // Per-segment tangent directions in NDC
        const int numSegs = poly.closed ? n : n - 1;

        auto segDir = [&](int si) -> V2
            {
                const int a = si % n;
                const int b = (si + 1) % n;
                return v2norm(v2sub(ndcXY(clip[b]), ndcXY(clip[a])));
            };

        auto segNorm = [&](int si) -> V2
            {
                return v2perp(segDir(si));
            };

        // Half-width offset for a given normal direction
        auto halfOffset = [&](V2 n2) -> V2
            {
                return { n2.x * halfWidthNdcX, n2.y * halfWidthNdcY };
            };

        // Compute the miter offset at vertex i between seg (i-1) and seg i
        //  - returns false (use bevel) if the miter would exceed miterLimit
        auto miterOffset = [&](int vi, int prevSeg, int nextSeg, V2& offsetOut) -> bool
            {
                const V2 n0 = segNorm(prevSeg);
                const V2 n1 = segNorm(nextSeg);

                // Miter direction = bisector of the two normals
                V2 miter = v2norm(v2add(n0, n1));

                // Length of miter vector so it reaches the outer corner:
                // 1 / dot(miter, n0)
                const float d = v2dot(miter, n0);
                if (std::abs(d) < 1e-6f)
                {
                    // Parallel segments — straight cap
                    offsetOut = halfOffset(n0);
                    return true;
                }

                const float miterLen = 1.0f / d;

                if (miterLen > style.miterLimit)
                    return false;   // caller should use bevel

                offsetOut = { miter.x * halfWidthNdcX * miterLen,
                              miter.y * halfWidthNdcY * miterLen };
                return true;
            };

        (void)miterOffset; // suppress warning if bevel-only path taken below

        // --- Emit segments and joins ---
        for (int si = 0; si < numSegs; ++si)
        {
            const V2 norm = segNorm(si);
            const V2 off = halfOffset(norm);

            const int vi0 = si % n;
            const int vi1 = (si + 1) % n;

            // Determine offsets at each end of this segment.
            // At an interior join we'll compute the miter; at endpoints use the segment normal.
            V2 off0 = off;
            V2 off1 = off;

            // Start join (not the very first vertex of an open polyline)
            const bool atStart = (si == 0 && !poly.closed);
            const bool atEnd = (si == numSegs - 1 && !poly.closed);

            if (!atStart)
            {
                const int prevSeg = (si - 1 + numSegs) % numSegs;
                if (style.join == LineJoin::Miter)
                {
                    V2 mo;
                    if (miterOffset(vi0, prevSeg, si, mo))
                        off0 = mo;
                    // else: miter too long, keep the segment normal (produces a slight gap
                    // that the bevel triangle below will fill on the prev segment's side)
                }
                // Bevel join triangle was emitted when processing prevSeg
            }

            if (!atEnd)
            {
                const int nextSeg = si + 1;
                if (style.join == LineJoin::Miter)
                {
                    V2 mo;
                    if (miterOffset(vi1, si, nextSeg, mo))
                        off1 = mo;
                }
            }

            emitSegmentQuad(out, clip[vi0], off0, clip[vi1], off1, edgeId);

            // Emit bevel triangle at the end of this segment if needed
            if (!atEnd || poly.closed)
            {
                const int nextSeg = (si + 1) % numSegs;
                if (style.join == LineJoin::Bevel)
                {
                    // Fill the gap between the outer corners of seg si and seg nextSeg
                    const V2 norm1 = segNorm(nextSeg);
                    const V2 offA = halfOffset(norm);   // outer corner from this seg end
                    const V2 offB = halfOffset(norm1);  // outer corner from next seg start

                    // Determine which side has a gap (the outside of the turn)
                    const V2 tA = segDir(si);
                    const V2 tB = segDir(nextSeg);
                    // Cross product sign tells us which side to fill
                    const float cross2d = tA.x * tB.y - tA.y * tB.x;

                    if (cross2d > 0.0f)
                        emitBevelTriangle(out, clip[vi1], offA, offB, edgeId);
                    else
                        emitBevelTriangle(out, clip[vi1],
                            { -offA.x, -offA.y }, { -offB.x, -offB.y }, edgeId);
                }
            }
        }
    }
}

EdgeQuadMeshCpu buildEdgeQuadMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    int viewportW,
    int viewportH,
    const EdgeStyle& style,
    float contentScaleX,
    float contentScaleY)
{
    EdgeQuadMeshCpu out;

    if (viewportW <= 0 || viewportH <= 0)
        return out;

    // Grow the quad by 1 physical pixel on each side to give the alpha fade room (for antialiasing)
    //  - the coverage coordinate is still normalised to the _original_ width, so opaque centre is unchanged
    const float growX = 1.0f / static_cast<float>(viewportW) * 2.0f * contentScaleX;
    const float growY = 1.0f / static_cast<float>(viewportH) * 2.0f * contentScaleY;

    const float halfW = (style.widthPx * contentScaleX * 0.5f)
        / static_cast<float>(viewportW) * 2.0f;
    const float halfH = (style.widthPx * contentScaleY * 0.5f)
        / static_cast<float>(viewportH) * 2.0f;

    const float halfWGrown = halfW + growX;
    const float halfHGrown = halfH + growY;

    for (const EdgePolyline3D& poly : polylines)
        expandPolyline(poly, viewProj, halfWGrown, halfHGrown, style, out);

    return out;
}
