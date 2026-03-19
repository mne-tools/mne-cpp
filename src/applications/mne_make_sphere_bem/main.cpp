//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Create a spherical BEM model by tessellating concentric spheres.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
/**
 * Create an icosphere by recursively subdividing an icosahedron.
 * Returns vertex positions (unit sphere) and triangle indices.
 */
static void makeIcosphere(int nSubdiv, MatrixX3f& verts, MatrixX3i& tris)
{
    // Initial icosahedron
    float t = (1.0f + sqrtf(5.0f)) / 2.0f;

    std::vector<Vector3f> v = {
        Vector3f(-1,  t, 0).normalized(), Vector3f( 1,  t, 0).normalized(),
        Vector3f(-1, -t, 0).normalized(), Vector3f( 1, -t, 0).normalized(),
        Vector3f( 0, -1,  t).normalized(), Vector3f( 0,  1,  t).normalized(),
        Vector3f( 0, -1, -t).normalized(), Vector3f( 0,  1, -t).normalized(),
        Vector3f( t,  0, -1).normalized(), Vector3f( t,  0,  1).normalized(),
        Vector3f(-t,  0, -1).normalized(), Vector3f(-t,  0,  1).normalized()
    };

    std::vector<Vector3i> f = {
        {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
        {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
        {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
        {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}
    };

    // Subdivision
    for (int s = 0; s < nSubdiv; ++s) {
        std::vector<Vector3i> newFaces;
        std::map<std::pair<int,int>, int> edgeMidpoint;

        auto getMidpoint = [&](int a, int b) -> int {
            auto key = std::make_pair(std::min(a, b), std::max(a, b));
            auto it = edgeMidpoint.find(key);
            if (it != edgeMidpoint.end()) return it->second;
            int idx = v.size();
            v.push_back(((v[a] + v[b]) * 0.5f).normalized());
            edgeMidpoint[key] = idx;
            return idx;
        };

        for (const auto& face : f) {
            int a = getMidpoint(face(0), face(1));
            int b = getMidpoint(face(1), face(2));
            int c = getMidpoint(face(2), face(0));
            newFaces.push_back(Vector3i(face(0), a, c));
            newFaces.push_back(Vector3i(face(1), b, a));
            newFaces.push_back(Vector3i(face(2), c, b));
            newFaces.push_back(Vector3i(a, b, c));
        }
        f = newFaces;
    }

    verts.resize(v.size(), 3);
    for (int i = 0; i < (int)v.size(); ++i)
        verts.row(i) = v[i].transpose();

    tris.resize(f.size(), 3);
    for (int i = 0; i < (int)f.size(); ++i)
        tris.row(i) = f[i].transpose();
}

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Create a spherical BEM model.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --out <file>       Output BEM FIFF file\n");
    fprintf(stderr, "  --origin <x,y,z>  Sphere origin in mm (default: 0,0,40)\n");
    fprintf(stderr, "  --radii <r1,r2,r3> Shell radii in mm (default: 71,80,90 for 3-layer)\n");
    fprintf(stderr, "  --conductivity <s1,s2,s3> Conductivities (default: 0.33,0.0042,0.33)\n");
    fprintf(stderr, "  --ico <n>          Icosahedron subdivision level (default: 4)\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString outFile;
    Vector3f origin(0.0f, 0.0f, 0.040f); // 40mm up, in meters
    QList<float> radii = {0.071f, 0.080f, 0.090f}; // default 3-layer, in meters
    QList<float> sigmas = {0.33f, 0.0042f, 0.33f};
    int icoLevel = 4;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--origin") == 0) {
            if (++k >= argc) { qCritical("--origin: argument required."); return 1; }
            QStringList parts = QString(argv[k]).split(',');
            if (parts.size() != 3) { qCritical("--origin: need x,y,z"); return 1; }
            origin = Vector3f(parts[0].toFloat() / 1000.0f,
                            parts[1].toFloat() / 1000.0f,
                            parts[2].toFloat() / 1000.0f);
        }
        else if (strcmp(argv[k], "--radii") == 0) {
            if (++k >= argc) { qCritical("--radii: argument required."); return 1; }
            radii.clear();
            QStringList parts = QString(argv[k]).split(',');
            for (const QString& p : parts) radii.append(p.toFloat() / 1000.0f); // mm to m
        }
        else if (strcmp(argv[k], "--conductivity") == 0) {
            if (++k >= argc) { qCritical("--conductivity: argument required."); return 1; }
            sigmas.clear();
            QStringList parts = QString(argv[k]).split(',');
            for (const QString& p : parts) sigmas.append(p.toFloat());
        }
        else if (strcmp(argv[k], "--ico") == 0) {
            if (++k >= argc) { qCritical("--ico: argument required."); return 1; }
            icoLevel = atoi(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }
    if (radii.size() != sigmas.size()) {
        qCritical("Number of radii (%d) must match number of conductivities (%d)",
                  radii.size(), sigmas.size());
        return 1;
    }

    // Create icosphere template
    MatrixX3f unitVerts;
    MatrixX3i tris;
    makeIcosphere(icoLevel, unitVerts, tris);
    printf("Icosphere subdivision %d: %d vertices, %d triangles\n",
           icoLevel, (int)unitVerts.rows(), (int)tris.rows());

    // Create BEM model
    MNEBem bem;
    QList<int> surfIds = { FIFFV_BEM_SURF_ID_BRAIN, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_HEAD };

    // Sort radii ascending (innermost first)
    QList<int> sortIdx;
    for (int i = 0; i < radii.size(); ++i) sortIdx.append(i);
    std::sort(sortIdx.begin(), sortIdx.end(), [&](int a, int b) { return radii[a] < radii[b]; });

    for (int i = 0; i < radii.size(); ++i) {
        int si = sortIdx[i];
        float radius = radii[si];
        float sigma = sigmas[si];
        int surfId = (i < surfIds.size()) ? surfIds[i] : FIFFV_BEM_SURF_ID_BRAIN;

        MNEBemSurface surf;
        surf.id = surfId;
        surf.np = unitVerts.rows();
        surf.ntri = tris.rows();
        surf.coord_frame = FIFFV_COORD_HEAD;
        surf.sigma = sigma;

        // Scale vertices to radius and translate to origin
        surf.rr = unitVerts * radius;
        for (int v = 0; v < surf.np; ++v) {
            surf.rr(v, 0) += origin(0);
            surf.rr(v, 1) += origin(1);
            surf.rr(v, 2) += origin(2);
        }

        surf.itris = tris;
        surf.nn = unitVerts; // normals point outward on unit sphere

        printf("Surface %d (%s): radius=%6.1f mm, sigma=%g S/m, %d vertices\n",
               i + 1, qPrintable(MNEBemSurface::id_name(surfId)),
               1000.0f * radius, sigma, surf.np);

        bem << surf;
    }

    // Write BEM
    QFile file(outFile);
    bem.write(file);
    printf("Written spherical BEM model to: %s\n", qPrintable(outFile));

    return 0;
}
