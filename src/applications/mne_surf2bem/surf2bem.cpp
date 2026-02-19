//=============================================================================================================
/**
 * @file     surf2bem.cpp
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
 * @brief    Surf2Bem class definition.
 *
 *           Ported from the original MNE C tool mne_surf2bem by Matti Hamalainen
 *           (SVN $Id: mne_surf2bem.c 3351 2012-03-05 12:03:50Z msh $).
 *
 *           Cross-referenced with MNE-Python's mne.write_bem_surfaces().
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surf2bem.h"
#include "mne_surf2bem_settings.h"
#include "surfacechecks.h"

#include <fs/surface.h>

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

#include <fiff/fiff_file.h>

#include <utils/ioutils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;
using namespace FSLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Surf2Bem::Surf2Bem(const MneSurf2BemSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int Surf2Bem::run()
{
    const QVector<SurfaceInput>& inputs = m_settings.surfaces();

    if (inputs.isEmpty()) {
        printf("Usage: mne_surf2bem [options]\n");
        printf("Use --help for available options.\n");
        return 1;
    }

    //
    // Report input summary
    //
    printf("\n");
    for (int k = 0; k < inputs.size(); ++k) {
        fprintf(stderr, "input  file # %3d : %s", k + 1, qPrintable(inputs[k].fileName));
        if (inputs[k].id > 0)
            fprintf(stderr, " / id = %d", inputs[k].id);
        else
            fprintf(stderr, " / id N/A");
        if (inputs[k].sigma > 0.0f)
            fprintf(stderr, " / sigma = %g S/m", inputs[k].sigma);
        else
            fprintf(stderr, " / sigma N/A");
        if (inputs[k].shift != 0.0f)
            fprintf(stderr, " / shift %6.1f mm", 1000.0f * inputs[k].shift);
        fprintf(stderr, "\n");
    }
    if (!m_settings.outputFile().isEmpty())
        fprintf(stderr, "output file       : %s\n", qPrintable(m_settings.outputFile()));
    fprintf(stderr, "\n");

    //
    // Step 1: Read all surfaces
    //
    QVector<MNEBemSurface> surfs;
    surfs.resize(inputs.size());

    for (int k = 0; k < inputs.size(); ++k) {
        bool ok;
        if (inputs[k].isAsciiTri) {
            ok = readAsciiTriSurface(inputs[k], surfs[k]);
            if (ok) {
                surfs[k].coord_frame = m_settings.coordFrame();
            }
        } else {
            ok = readFreeSurferSurface(inputs[k], surfs[k]);
        }

        if (!ok) {
            qCritical() << "Failed to read surface" << inputs[k].fileName;
            return 1;
        }

        surfs[k].id = inputs[k].id;
        fprintf(stderr, "%s read. id = %d\n\n", qPrintable(inputs[k].fileName), inputs[k].id);
    }

    //
    // Step 2: Shift vertices along normals if requested
    //
    for (int k = 0; k < inputs.size(); ++k) {
        if (inputs[k].shift != 0.0f) {
            if (!shiftVertices(surfs[k], inputs[k].shift)) {
                return 1;
            }
        }
    }

    //
    // Step 3: Order surfaces for topology checks (head > skull > brain)
    //
    orderSurfaces(surfs);

    //
    // Step 4: Topology checks
    //
    if (m_settings.check() && (surfs.size() == 3 || surfs.size() == 1)) {
        if (!SurfaceChecks::checkSurfaces(surfs)) {
            return 1;
        }
        for (int k = 0; k < surfs.size(); ++k) {
            if (!SurfaceChecks::checkSurfaceSize(surfs[k])) {
                return 1;
            }
        }
        fprintf(stderr, "Surfaces passed the basic topology checks.\n");

        if (m_settings.checkMore()) {
            if (!SurfaceChecks::checkThicknesses(surfs)) {
                return 1;
            }
        }
    } else {
        fprintf(stderr, "Topology checks skipped.\n");
    }

    //
    // Step 5: Write BEM FIFF output
    //
    if (!m_settings.outputFile().isEmpty()) {
        // Build MNEBem from surfaces, applying conductivities
        MNEBem bem;
        for (int k = 0; k < surfs.size(); ++k) {
            if (inputs[k].sigma > 0.0f) {
                surfs[k].sigma = inputs[k].sigma;
            }
            bem << surfs[k];
        }

        QFile file(m_settings.outputFile());

        bem.write(file);

        fprintf(stderr, "%s written.\n", qPrintable(m_settings.outputFile()));
    } else {
        fprintf(stderr, "No output requested.\n");
    }

    return 0;
}

//=============================================================================================================

bool Surf2Bem::readFreeSurferSurface(const SurfaceInput& input, MNEBemSurface& bemSurf)
{
    //
    // Read FreeSurfer binary surface directly.
    //
    // We cannot use FSLIB::Surface::read() because it requires the filename
    // to contain "lh." or "rh." (hemisphere prefix). BEM surface files like
    // inner_skull.surf do not have this prefix.
    //
    // Magic numbers:
    //   TRIANGLE_FILE_MAGIC_NUMBER = 0xFFFFFE = 16777214
    //   QUAD_FILE_MAGIC_NUMBER     = 0xFFFFFF = 16777215
    //   NEW_QUAD_FILE_MAGIC_NUMBER = 0xFFFFFD = 16777213
    //
    QFile file(input.fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not open surface file:" << input.fileName;
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::BigEndian);

    const qint32 TRIANGLE_FILE_MAGIC = 16777214;

    qint32 magic = UTILSLIB::IOUtils::fread3(stream);

    qint32 nvert = 0;
    qint32 nface = 0;
    MatrixXf verts;
    MatrixXi faces;

    if (magic == TRIANGLE_FILE_MAGIC) {
        // Skip the two comment lines (terminated by \n)
        file.readLine();
        file.readLine();

        stream >> nvert;
        stream >> nface;

        fprintf(stderr, "%s: triangle file with %d vertices and %d triangles\n",
                qPrintable(input.fileName), nvert, nface);

        // Read vertices (stored as 3 x nvert, column-major float32 big-endian)
        verts.resize(3, nvert);
        stream.readRawData(reinterpret_cast<char*>(verts.data()), nvert * 3 * sizeof(float));
        for (qint32 i = 0; i < 3; ++i)
            for (qint32 j = 0; j < nvert; ++j)
                UTILSLIB::IOUtils::swap_floatp(&verts(i, j));

        // Read faces (nface x 3, int32)
        faces.resize(nface, 3);
        for (qint32 i = 0; i < nface; ++i) {
            for (qint32 j = 0; j < 3; ++j) {
                stream >> faces(i, j);
            }
        }
    } else {
        qCritical() << "Unsupported surface file format (magic =" << magic << ") in" << input.fileName;
        qCritical() << "Only FreeSurfer triangle format (0xFFFFFE) is supported for BEM surfaces.";
        return false;
    }

    file.close();

    //
    // Populate BEM surface
    // Vertices are in mm in the file, convert to meters (matching FSLIB::Surface behavior)
    //
    verts.transposeInPlace();  // now nvert x 3
    verts.array() *= 0.001f;

    bemSurf.np = nvert;
    bemSurf.ntri = nface;
    bemSurf.coord_frame = FIFFV_COORD_MRI;
    bemSurf.rr = verts.block(0, 0, nvert, 3);
    bemSurf.tris = faces.block(0, 0, nface, 3);

    // Compute vertex normals
    bemSurf.nn = Surface::compute_normals(bemSurf.rr, bemSurf.tris);

    fprintf(stderr, "Read FreeSurfer surface: %d vertices, %d triangles\n",
            bemSurf.np, bemSurf.ntri);

    return true;
}

//=============================================================================================================

bool Surf2Bem::readAsciiTriSurface(const SurfaceInput& input, MNEBemSurface& bemSurf)
{
    //
    // Read ASCII triangle file
    //
    // Format:
    //   nvert
    //   x1 y1 z1
    //   x2 y2 z2
    //   ...
    //   ntri
    //   v1 v2 v3   (1-based vertex indices)
    //   ...
    //
    QFile file(input.fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not open ASCII triangle file:" << input.fileName;
        return false;
    }

    QTextStream in(&file);

    // Read number of vertices
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        qCritical() << "Invalid vertex count in" << input.fileName;
        return false;
    }

    // Read vertices
    MatrixX3f rr(nvert, 3);
    for (int k = 0; k < nvert; ++k) {
        float x, y, z;
        in >> x >> y >> z;
        if (in.status() != QTextStream::Ok) {
            qCritical() << "Error reading vertex" << k << "from" << input.fileName;
            return false;
        }
        rr(k, 0) = x;
        rr(k, 1) = y;
        rr(k, 2) = z;
    }

    // Read number of triangles
    int ntri = 0;
    in >> ntri;
    if (ntri <= 0) {
        qCritical() << "Invalid triangle count in" << input.fileName;
        return false;
    }

    // Read triangles (1-based in file, convert to 0-based)
    MatrixX3i tris(ntri, 3);
    for (int k = 0; k < ntri; ++k) {
        int v1, v2, v3;
        in >> v1 >> v2 >> v3;
        if (in.status() != QTextStream::Ok) {
            qCritical() << "Error reading triangle" << k << "from" << input.fileName;
            return false;
        }
        if (input.swap) {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v3 - 1;   // Swapped
            tris(k, 2) = v2 - 1;   // Swapped
        } else {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v2 - 1;
            tris(k, 2) = v3 - 1;
        }
    }
    file.close();

    //
    // Convert units: if mm==true (default), convert from mm to meters
    //
    if (input.mm) {
        rr /= 1000.0f;
    }

    //
    // Populate BEM surface
    //
    bemSurf.np = nvert;
    bemSurf.ntri = ntri;
    bemSurf.rr = rr;
    bemSurf.tris = tris;

    // Compute vertex normals
    bemSurf.nn = Surface::compute_normals(bemSurf.rr, bemSurf.tris);

    fprintf(stderr, "Read ASCII triangle surface: %d vertices, %d triangles\n",
            bemSurf.np, bemSurf.ntri);

    return true;
}

//=============================================================================================================

bool Surf2Bem::shiftVertices(MNEBemSurface& surf, float shift)
{
    if (shift == 0.0f)
        return true;

    //
    // Shift vertices outward along normals
    //
    for (int k = 0; k < surf.np; ++k) {
        surf.rr(k, 0) += shift * surf.nn(k, 0);
        surf.rr(k, 1) += shift * surf.nn(k, 1);
        surf.rr(k, 2) += shift * surf.nn(k, 2);
    }

    // Recompute normals after shifting
    surf.nn = Surface::compute_normals(surf.rr, surf.tris);

    fprintf(stderr, "%s vertex locations shifted by %6.1f mm.\n",
            qPrintable(SurfaceChecks::getNameOf(surf.id)),
            1000.0f * shift);

    return true;
}

//=============================================================================================================

void Surf2Bem::orderSurfaces(QVector<MNEBemSurface>& surfs)
{
    //
    // Reorder to head, skull, brain for proper BEM nesting
    // Only reorder if all three types are present
    //
    if (surfs.size() != 3)
        return;

    int headIdx = -1, skullIdx = -1, brainIdx = -1;
    for (int k = 0; k < 3; ++k) {
        if (surfs[k].id == FIFFV_BEM_SURF_ID_HEAD)
            headIdx = k;
        else if (surfs[k].id == FIFFV_BEM_SURF_ID_SKULL)
            skullIdx = k;
        else if (surfs[k].id == FIFFV_BEM_SURF_ID_BRAIN)
            brainIdx = k;
    }

    if (headIdx >= 0 && skullIdx >= 0 && brainIdx >= 0) {
        QVector<MNEBemSurface> ordered(3);
        ordered[0] = surfs[headIdx];
        ordered[1] = surfs[skullIdx];
        ordered[2] = surfs[brainIdx];
        surfs = ordered;
    }
}
