//=============================================================================================================
/**
 * @file     meshfactory.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MeshFactory class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "meshfactory.h"
#include <Eigen/Geometry>
#include <QMap>
#include <QPair>
#include <cmath>

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace {

Eigen::Matrix3f extractRotation(const QMatrix4x4 &m)
{
    Eigen::Matrix3f rot;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            rot(r, c) = m(r, c);
    return rot;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MeshFactory::buildIcosphere(QVector<Eigen::Vector3f> &vertices,
                                  QVector<Eigen::Vector3i> &faces,
                                  int subdivisions)
{
    // Golden ratio
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    vertices.clear();
    vertices.reserve(12 + 30 * subdivisions); // rough estimate
    vertices << Eigen::Vector3f(-1,  phi, 0) << Eigen::Vector3f( 1,  phi, 0)
             << Eigen::Vector3f(-1, -phi, 0) << Eigen::Vector3f( 1, -phi, 0)
             << Eigen::Vector3f( 0, -1,  phi) << Eigen::Vector3f( 0,  1,  phi)
             << Eigen::Vector3f( 0, -1, -phi) << Eigen::Vector3f( 0,  1, -phi)
             << Eigen::Vector3f( phi, 0, -1) << Eigen::Vector3f( phi, 0,  1)
             << Eigen::Vector3f(-phi, 0, -1) << Eigen::Vector3f(-phi, 0,  1);
    for (auto &v : vertices)
        v.normalize();

    faces.clear();
    faces.reserve(20);
    faces << Eigen::Vector3i(0,11,5)  << Eigen::Vector3i(0,5,1)
          << Eigen::Vector3i(0,1,7)   << Eigen::Vector3i(0,7,10)
          << Eigen::Vector3i(0,10,11) << Eigen::Vector3i(1,5,9)
          << Eigen::Vector3i(5,11,4)  << Eigen::Vector3i(11,10,2)
          << Eigen::Vector3i(10,7,6)  << Eigen::Vector3i(7,1,8)
          << Eigen::Vector3i(3,9,4)   << Eigen::Vector3i(3,4,2)
          << Eigen::Vector3i(3,2,6)   << Eigen::Vector3i(3,6,8)
          << Eigen::Vector3i(3,8,9)   << Eigen::Vector3i(4,9,5)
          << Eigen::Vector3i(2,4,11)  << Eigen::Vector3i(6,2,10)
          << Eigen::Vector3i(8,6,7)   << Eigen::Vector3i(9,8,1);

    // Subdivide
    for (int s = 0; s < subdivisions; ++s) {
        QMap<QPair<int,int>, int> cache;
        auto midpoint = [&](int a, int b) -> int {
            auto key = qMakePair(qMin(a, b), qMax(a, b));
            if (cache.contains(key))
                return cache[key];
            int idx = vertices.size();
            vertices.append((vertices[a] + vertices[b]).normalized());
            cache[key] = idx;
            return idx;
        };

        QVector<Eigen::Vector3i> newFaces;
        newFaces.reserve(faces.size() * 4);
        for (const auto &f : faces) {
            int ab = midpoint(f(0), f(1));
            int bc = midpoint(f(1), f(2));
            int ca = midpoint(f(2), f(0));
            newFaces << Eigen::Vector3i(f(0), ab, ca)
                     << Eigen::Vector3i(f(1), bc, ab)
                     << Eigen::Vector3i(f(2), ca, bc)
                     << Eigen::Vector3i(ab,   bc, ca);
        }
        faces = std::move(newFaces);
    }
}

//=============================================================================================================

int MeshFactory::sphereVertexCount(int subdivisions)
{
    QVector<Eigen::Vector3f> verts;
    QVector<Eigen::Vector3i> faces;
    buildIcosphere(verts, faces, subdivisions);
    return verts.size();
}

//=============================================================================================================

std::shared_ptr<BrainSurface> MeshFactory::createSphere(const QVector3D &center,
                                                          float radius,
                                                          const QColor &color,
                                                          int subdivisions)
{
    QVector<Eigen::Vector3f> verts;
    QVector<Eigen::Vector3i> faces;
    buildIcosphere(verts, faces, subdivisions);

    const int nV = verts.size();
    const int nT = faces.size();

    Eigen::MatrixX3f rr(nV, 3), nn(nV, 3);
    for (int i = 0; i < nV; ++i) {
        nn(i, 0) = verts[i].x();
        nn(i, 1) = verts[i].y();
        nn(i, 2) = verts[i].z();
        rr(i, 0) = verts[i].x() * radius + center.x();
        rr(i, 1) = verts[i].y() * radius + center.y();
        rr(i, 2) = verts[i].z() * radius + center.z();
    }

    Eigen::MatrixX3i tris(nT, 3);
    for (int i = 0; i < nT; ++i)
        tris.row(i) = faces[i];

    auto surf = std::make_shared<BrainSurface>();
    surf->createFromData(rr, nn, tris, color);
    return surf;
}

//=============================================================================================================

std::shared_ptr<BrainSurface> MeshFactory::createPlate(const QVector3D &center,
                                                         const QMatrix4x4 &orientation,
                                                         const QColor &color,
                                                         float size)
{
    const float hw = size / 2.0f;
    const float hh = size / 2.0f;
    const float hd = size * 0.05f; // thin plate

    const Eigen::Matrix3f rot = extractRotation(orientation);

    Eigen::Vector3f corners[8] = {
        {-hw, -hh, -hd}, { hw, -hh, -hd}, { hw,  hh, -hd}, {-hw,  hh, -hd},
        {-hw, -hh,  hd}, { hw, -hh,  hd}, { hw,  hh,  hd}, {-hw,  hh,  hd}
    };
    for (auto &c : corners)
        c = rot * c;

    const int faceIndices[6][4] = {
        {4,5,6,7}, {1,0,3,2},
        {3,7,6,2}, {0,1,5,4},
        {1,2,6,5}, {0,4,7,3}
    };

    Eigen::MatrixX3f rr(24, 3), nn(24, 3);
    Eigen::MatrixX3i tris(12, 3);

    for (int f = 0; f < 6; ++f) {
        Eigen::Vector3f v0 = corners[faceIndices[f][0]];
        Eigen::Vector3f v1 = corners[faceIndices[f][1]];
        Eigen::Vector3f v2 = corners[faceIndices[f][2]];
        Eigen::Vector3f e1 = v1 - v0;
        Eigen::Vector3f e2 = v2 - v0;
        Eigen::Vector3f fn = e1.cross(e2).normalized();
        int base = f * 4;
        for (int k = 0; k < 4; ++k) {
            Eigen::Vector3f v = corners[faceIndices[f][k]];
            rr.row(base + k) << v.x() + center.x(), v.y() + center.y(), v.z() + center.z();
            nn.row(base + k) = fn;
        }
        tris.row(f * 2)     << base, base + 1, base + 2;
        tris.row(f * 2 + 1) << base, base + 2, base + 3;
    }

    auto surf = std::make_shared<BrainSurface>();
    surf->createFromData(rr, nn, tris, color);
    return surf;
}

//=============================================================================================================

std::shared_ptr<BrainSurface> MeshFactory::createBarbell(const QVector3D &center,
                                                           const QMatrix4x4 &orientation,
                                                           const QColor &color,
                                                           float size)
{
    const Eigen::Matrix3f rot = extractRotation(orientation);

    const float halfSpan = size * 0.6f;
    const float sphereR  = size * 0.2f;
    const float rodR     = size * 0.08f;

    QVector<Eigen::Vector3f> allVerts;
    QVector<Eigen::Vector3f> allNorms;
    QVector<Eigen::Vector3i> allTris;

    // ── Helper: append a sphere ────────────────────────────────────────
    auto appendSphere = [&](const QVector3D &pos, float radius) {
        QVector<Eigen::Vector3f> sv;
        QVector<Eigen::Vector3i> sf;
        buildIcosphere(sv, sf, 1); // single subdivision

        const int base = allVerts.size();
        allVerts.reserve(base + sv.size());
        allNorms.reserve(base + sv.size());
        allTris.reserve(allTris.size() + sf.size());

        for (const auto &v : sv) {
            Eigen::Vector3f vn = rot * v;
            Eigen::Vector3f vp = rot * (v * radius);
            allVerts.append(Eigen::Vector3f(vp.x() + pos.x(), vp.y() + pos.y(), vp.z() + pos.z()));
            allNorms.append(vn.normalized());
        }
        for (const auto &f : sf) {
            allTris.append(Eigen::Vector3i(base + f(0), base + f(1), base + f(2)));
        }
    };

    // ── Helper: append a box-rod ───────────────────────────────────────
    auto appendRod = [&]() {
        float hx = rodR, hy = halfSpan, hz = rodR;
        Eigen::Vector3f corners[8] = {
            {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
            {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz}
        };
        for (auto &c : corners) c = rot * c;

        const int faceIdx[6][4] = {
            {4,5,6,7}, {1,0,3,2}, {3,7,6,2}, {0,1,5,4}, {1,2,6,5}, {0,4,7,3}
        };

        const int base = allVerts.size();
        for (int f = 0; f < 6; ++f) {
            Eigen::Vector3f v0 = corners[faceIdx[f][0]];
            Eigen::Vector3f v1 = corners[faceIdx[f][1]];
            Eigen::Vector3f v2 = corners[faceIdx[f][2]];
            Eigen::Vector3f e1 = v1 - v0;
            Eigen::Vector3f e2 = v2 - v0;
            Eigen::Vector3f fn = e1.cross(e2).normalized();
            int faceBase = base + f * 4;
            for (int k = 0; k < 4; ++k) {
                Eigen::Vector3f v = corners[faceIdx[f][k]];
                allVerts.append(Eigen::Vector3f(v.x() + center.x(), v.y() + center.y(), v.z() + center.z()));
                allNorms.append(fn);
            }
            allTris.append(Eigen::Vector3i(faceBase, faceBase + 1, faceBase + 2));
            allTris.append(Eigen::Vector3i(faceBase, faceBase + 2, faceBase + 3));
        }
    };

    // Build the barbell
    QVector3D axis(rot(0,1), rot(1,1), rot(2,1));
    appendSphere(center + axis * halfSpan, sphereR);
    appendSphere(center - axis * halfSpan, sphereR);
    appendRod();

    // Pack into Eigen matrices
    Eigen::MatrixX3f rr(allVerts.size(), 3), nn(allNorms.size(), 3);
    Eigen::MatrixX3i tt(allTris.size(), 3);
    for (int i = 0; i < allVerts.size(); ++i) {
        rr.row(i) = allVerts[i].transpose();
        nn.row(i) = allNorms[i].transpose();
    }
    for (int i = 0; i < allTris.size(); ++i)
        tt.row(i) = allTris[i];

    auto surf = std::make_shared<BrainSurface>();
    surf->createFromData(rr, nn, tt, color);
    return surf;
}

//=============================================================================================================

std::shared_ptr<BrainSurface> MeshFactory::createBatchedSpheres(const QVector<QVector3D> &positions,
                                                                  float radius,
                                                                  const QColor &color,
                                                                  int subdivisions)
{
    if (positions.isEmpty())
        return std::make_shared<BrainSurface>();

    // Build template sphere once
    QVector<Eigen::Vector3f> tmplVerts;
    QVector<Eigen::Vector3i> tmplFaces;
    buildIcosphere(tmplVerts, tmplFaces, subdivisions);

    const int nVPerPt = tmplVerts.size();
    const int nTPerPt = tmplFaces.size();
    const int nPts = positions.size();

    // Scale template
    Eigen::MatrixX3f templateV(nVPerPt, 3), templateN(nVPerPt, 3);
    for (int i = 0; i < nVPerPt; ++i) {
        templateN(i, 0) = tmplVerts[i].x();
        templateN(i, 1) = tmplVerts[i].y();
        templateN(i, 2) = tmplVerts[i].z();
        templateV(i, 0) = tmplVerts[i].x() * radius;
        templateV(i, 1) = tmplVerts[i].y() * radius;
        templateV(i, 2) = tmplVerts[i].z() * radius;
    }
    Eigen::MatrixX3i templateT(nTPerPt, 3);
    for (int i = 0; i < nTPerPt; ++i)
        templateT.row(i) = tmplFaces[i];

    // Merge all spheres into single mesh
    Eigen::MatrixX3f allVerts(nPts * nVPerPt, 3);
    Eigen::MatrixX3f allNorms(nPts * nVPerPt, 3);
    Eigen::MatrixX3i allTris(nPts * nTPerPt, 3);

    for (int p = 0; p < nPts; ++p) {
        const QVector3D &pos = positions[p];
        const int vOff = p * nVPerPt;
        const int tOff = p * nTPerPt;
        for (int iv = 0; iv < nVPerPt; ++iv) {
            allVerts(vOff + iv, 0) = templateV(iv, 0) + pos.x();
            allVerts(vOff + iv, 1) = templateV(iv, 1) + pos.y();
            allVerts(vOff + iv, 2) = templateV(iv, 2) + pos.z();
            allNorms.row(vOff + iv) = templateN.row(iv);
        }
        for (int it = 0; it < nTPerPt; ++it) {
            allTris(tOff + it, 0) = templateT(it, 0) + vOff;
            allTris(tOff + it, 1) = templateT(it, 1) + vOff;
            allTris(tOff + it, 2) = templateT(it, 2) + vOff;
        }
    }

    auto surf = std::make_shared<BrainSurface>();
    surf->createFromData(allVerts, allNorms, allTris, color);
    return surf;
}
