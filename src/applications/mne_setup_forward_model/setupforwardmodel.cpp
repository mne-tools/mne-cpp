//=============================================================================================================
/**
 * @file     setupforwardmodel.cpp
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
 * @brief    SetupForwardModel class definition.
 *
 *           Ported from the original MNE shell script mne_setup_forward_model
 *           by Matti Hamalainen (SVN $Id: mne_setup_forward_model 3282
 *           2011-02-02 14:28:16Z gramfort $).
 *
 *           The original script orchestrated three external programs:
 *             1. mne_surf2bem — create BEM geometry FIFF file
 *             2. mne_list_bem — export .pnt and .surf files
 *             3. mne_prepare_bem_model — compute BEM solution matrix
 *
 *           This C++ port uses mne-cpp library classes directly for all
 *           three steps, eliminating the need for external tools.
 *
 *           Cross-referenced with MNE-Python's mne.make_bem_model() and
 *           mne.make_bem_solution().
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "setupforwardmodel.h"
#include "mne_setup_forward_model_settings.h"

#include <fs/surface.h>

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>

#include <fwd/fwd_bem_model.h>

#include <mne/c/mne_surface_old.h>
#include <mne/c/mne_triangle.h>

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

using namespace MNESETUPFORWARDMODEL;
using namespace FSLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace FWDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SetupForwardModel::SetupForwardModel(const MneSetupForwardModelSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int SetupForwardModel::run()
{
    //
    // Validate settings
    //
    if (m_settings.subjectsDir().isEmpty()) {
        qCritical() << "The environment variable SUBJECTS_DIR should be set.";
        qCritical() << "Use --subjects-dir or set the SUBJECTS_DIR environment variable.";
        return 1;
    }
    if (m_settings.subject().isEmpty()) {
        qCritical() << "Subject not specified.";
        qCritical() << "Use --subject or set the SUBJECT environment variable.";
        return 1;
    }

    //
    // Derive paths
    //
    QString subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    QString bemDir = subjectDir + "/bem";
    QString ext = m_settings.useSurfFormat() ? "surf" : "tri";

    if (!QDir(subjectDir).exists()) {
        qCritical() << "Could not find the MRI data directory" << subjectDir;
        return 1;
    }
    if (!QDir(bemDir).exists()) {
        qCritical() << "Could not find the BEM directory" << bemDir;
        return 1;
    }

    //
    // Locate surface files
    //
    QString innerSkullFile;
    if (!locateSurface(bemDir, "inner_skull", ext, innerSkullFile)) {
        qCritical() << "Could not find the inner skull triangulation";
        return 1;
    }

    QString outerSkullFile, outerSkinFile;
    if (!m_settings.homogeneous()) {
        if (!locateSurface(bemDir, "outer_skull", ext, outerSkullFile)) {
            qCritical() << "Could not find the outer skull triangulation" << outerSkullFile;
            return 1;
        }
        if (!locateSurface(bemDir, "outer_skin", ext, outerSkinFile)) {
            qCritical() << "Could not find the scalp triangulation" << outerSkinFile;
            return 1;
        }
    }

    //
    // Step 1: Read surfaces and create BEM geometry
    //
    printf("\n");
    printf("Setting up the BEM with the following parameters:\n");
    printf("\n");
    printf("SUBJECTS_DIR       = %s\n", qPrintable(m_settings.subjectsDir()));
    printf("Subject            = %s\n", qPrintable(m_settings.subject()));

    // Read inner skull
    MNEBemSurface innerSkull;
    if (m_settings.useSurfFormat()) {
        if (!readFreeSurferSurf(innerSkullFile, FIFFV_BEM_SURF_ID_BRAIN,
                                m_settings.brainConductivity(),
                                m_settings.innerShift(), innerSkull)) {
            return 1;
        }
    } else {
        if (!readAsciiTriFile(innerSkullFile, FIFFV_BEM_SURF_ID_BRAIN,
                              m_settings.brainConductivity(),
                              m_settings.innerShift(), innerSkull)) {
            return 1;
        }
    }
    printf("Inner skull        = %s (%d triangles)\n",
           qPrintable(innerSkullFile), innerSkull.ntri);

    MNEBemSurface outerSkull, outerSkin;
    if (!m_settings.homogeneous()) {
        // Read outer skull
        if (m_settings.useSurfFormat()) {
            if (!readFreeSurferSurf(outerSkullFile, FIFFV_BEM_SURF_ID_SKULL,
                                    m_settings.skullConductivity(),
                                    m_settings.outerShift(), outerSkull)) {
                return 1;
            }
        } else {
            if (!readAsciiTriFile(outerSkullFile, FIFFV_BEM_SURF_ID_SKULL,
                                  m_settings.skullConductivity(),
                                  m_settings.outerShift(), outerSkull)) {
                return 1;
            }
        }
        printf("Outer skull        = %s (%d triangles)\n",
               qPrintable(outerSkullFile), outerSkull.ntri);

        // Read outer skin (scalp)
        if (m_settings.useSurfFormat()) {
            if (!readFreeSurferSurf(outerSkinFile, FIFFV_BEM_SURF_ID_HEAD,
                                    m_settings.scalpConductivity(),
                                    m_settings.scalpShift(), outerSkin)) {
                return 1;
            }
        } else {
            if (!readAsciiTriFile(outerSkinFile, FIFFV_BEM_SURF_ID_HEAD,
                                  m_settings.scalpConductivity(),
                                  m_settings.scalpShift(), outerSkin)) {
                return 1;
            }
        }
        printf("Scalp              = %s (%d triangles)\n",
               qPrintable(outerSkinFile), outerSkin.ntri);
    }

    printf("brain conductivity = %g S/m\n", m_settings.brainConductivity());
    if (!m_settings.homogeneous()) {
        printf("skull conductivity = %g S/m\n", m_settings.skullConductivity());
        printf("scalp conductivity = %g S/m\n", m_settings.scalpConductivity());
    }

    //
    // Figure out the model name
    //
    QString modelFile;
    if (m_settings.modelName().isEmpty()) {
        if (m_settings.homogeneous()) {
            modelFile = QString("%1/%2-%3-bem.fif")
                .arg(bemDir)
                .arg(m_settings.subject())
                .arg(innerSkull.ntri);
        } else {
            modelFile = QString("%1/%2-%3-%4-%5-bem.fif")
                .arg(bemDir)
                .arg(m_settings.subject())
                .arg(innerSkull.ntri)
                .arg(outerSkull.ntri)
                .arg(outerSkin.ntri);
        }
    } else {
        modelFile = bemDir + "/" + m_settings.modelName() + "-bem.fif";
    }

    printf("Resulting BEM      = %s\n\n", qPrintable(modelFile));

    //
    // Check if file already exists
    //
    if (QFileInfo::exists(modelFile) && !m_settings.overwrite()) {
        qCritical() << "Output file" << modelFile << "already exists.";
        qCritical() << "Use --overwrite to overwrite.";
        return 1;
    }

    //
    // >> 1. Creating the BEM geometry file
    //
    printf(">> 1. Creating the BEM geometry file...\n");

    MNEBem bem;
    if (m_settings.homogeneous()) {
        bem << innerSkull;
    } else {
        // Order: head (4), skull (3), brain (1) — standard BEM nesting
        bem << outerSkin;
        bem << outerSkull;
        bem << innerSkull;
    }

    {
        QFile file(modelFile);
        bem.write(file);
    }
    printf("BEM geometry file written to %s\n\n", qPrintable(modelFile));

    //
    // >> 2. Creating ascii pnt files and surf files
    //
    printf(">> 2. Creating ascii pnt files and surf files...\n");

    if (m_settings.homogeneous()) {
        QString pntFile = QString("%1/%2-inner_skull-%3.pnt")
            .arg(bemDir).arg(m_settings.subject()).arg(innerSkull.ntri);
        QString surfFile = QString("%1/%2-inner_skull-%3.surf")
            .arg(bemDir).arg(m_settings.subject()).arg(innerSkull.ntri);

        if (!exportPntFile(innerSkull, pntFile)) return 1;
        if (!exportSurfFile(innerSkull, surfFile)) return 1;
    } else {
        // Outer skin (scalp)
        {
            QString pntFile = QString("%1/%2-outer_skin-%3.pnt")
                .arg(bemDir).arg(m_settings.subject()).arg(outerSkin.ntri);
            QString surfFile = QString("%1/%2-outer_skin-%3.surf")
                .arg(bemDir).arg(m_settings.subject()).arg(outerSkin.ntri);

            if (!exportPntFile(outerSkin, pntFile)) return 1;
            if (!exportSurfFile(outerSkin, surfFile)) return 1;
        }

        // Outer skull
        {
            QString pntFile = QString("%1/%2-outer_skull-%3.pnt")
                .arg(bemDir).arg(m_settings.subject()).arg(outerSkull.ntri);
            QString surfFile = QString("%1/%2-outer_skull-%3.surf")
                .arg(bemDir).arg(m_settings.subject()).arg(outerSkull.ntri);

            if (!exportPntFile(outerSkull, pntFile)) return 1;
            if (!exportSurfFile(outerSkull, surfFile)) return 1;
        }

        // Inner skull
        {
            QString pntFile = QString("%1/%2-inner_skull-%3.pnt")
                .arg(bemDir).arg(m_settings.subject()).arg(innerSkull.ntri);
            QString surfFile = QString("%1/%2-inner_skull-%3.surf")
                .arg(bemDir).arg(m_settings.subject()).arg(innerSkull.ntri);

            if (!exportPntFile(innerSkull, pntFile)) return 1;
            if (!exportSurfFile(innerSkull, surfFile)) return 1;
        }
    }
    printf("\n");

    //
    // >> 3. Calculating BEM geometry data (solution)
    //
    if (!m_settings.noSolution()) {
        printf(">> 3. Calculating BEM geometry data (this takes several minutes)...\n\n");

        QString solFile = modelFile;
        solFile.replace(".fif", "-sol.fif");

        if (!prepareBemSolution(modelFile, solFile)) {
            qCritical() << "Model preparation failed.";
            return 1;
        }  else {
            printf("\nThe model %s is now ready for use\n", qPrintable(solFile));
        }
    }

    printf("\nComplete.\n");
    return 0;
}

//=============================================================================================================

bool SetupForwardModel::locateSurface(const QString& bemDir, const QString& name,
                                      const QString& ext, QString& path) const
{
    // Try: <bem_dir>/<name>.<ext>
    path = bemDir + "/" + name + "." + ext;
    if (QFileInfo::exists(path)) {
        return true;
    }

    // Try: <bem_dir>/<subject>-<name>.<ext>
    QString alt = bemDir + "/" + m_settings.subject() + "-" + name + "." + ext;
    if (QFileInfo::exists(alt)) {
        path = alt;
        return true;
    }

    // Not found
    return false;
}

//=============================================================================================================

bool SetupForwardModel::readAsciiTriFile(const QString& fileName, int id, float sigma,
                                         float shift, MNEBemSurface& surf) const
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
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not open ASCII triangle file:" << fileName;
        return false;
    }

    QTextStream in(&file);

    // Read number of vertices
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        qCritical() << "Invalid vertex count in" << fileName;
        return false;
    }

    // Read vertices
    MatrixX3f rr(nvert, 3);
    for (int k = 0; k < nvert; ++k) {
        float x, y, z;
        in >> x >> y >> z;
        if (in.status() != QTextStream::Ok) {
            qCritical() << "Error reading vertex" << k << "from" << fileName;
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
        qCritical() << "Invalid triangle count in" << fileName;
        return false;
    }

    // Read triangles (1-based in file, convert to 0-based)
    MatrixX3i tris(ntri, 3);
    for (int k = 0; k < ntri; ++k) {
        int v1, v2, v3;
        in >> v1 >> v2 >> v3;
        if (in.status() != QTextStream::Ok) {
            qCritical() << "Error reading triangle" << k << "from" << fileName;
            return false;
        }
        if (m_settings.swap()) {
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
    // Convert units: default is mm, --meters means already in meters
    //
    if (!m_settings.meters()) {
        rr /= 1000.0f;
    }

    //
    // Populate BEM surface
    //
    surf.np = nvert;
    surf.ntri = ntri;
    surf.coord_frame = FIFFV_COORD_MRI;
    surf.rr = rr;
    surf.tris = tris;
    surf.id = id;
    surf.sigma = sigma;

    // Compute vertex normals
    surf.nn = Surface::compute_normals(surf.rr, surf.tris);

    // Shift vertices if requested
    if (shift != 0.0f) {
        shiftVertices(surf, shift / 1000.0f);   // Convert mm to meters
    }

    return true;
}

//=============================================================================================================

bool SetupForwardModel::readFreeSurferSurf(const QString& fileName, int id, float sigma,
                                           float shift, MNEBemSurface& surf) const
{
    //
    // Read FreeSurfer binary surface file directly
    //
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not open surface file:" << fileName;
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::BigEndian);

    const qint32 TRIANGLE_FILE_MAGIC = 16777214;

    qint32 magic = UTILSLIB::IOUtils::fread3(stream);

    if (magic != TRIANGLE_FILE_MAGIC) {
        qCritical() << "Unsupported surface file format (magic =" << magic << ") in" << fileName;
        return false;
    }

    // Skip two comment lines
    file.readLine();
    file.readLine();

    qint32 nvert = 0, nface = 0;
    stream >> nvert;
    stream >> nface;

    // Read vertices (stored as 3 x nvert, column-major float32 big-endian)
    MatrixXf verts(3, nvert);
    stream.readRawData(reinterpret_cast<char*>(verts.data()), nvert * 3 * sizeof(float));
    for (qint32 i = 0; i < 3; ++i)
        for (qint32 j = 0; j < nvert; ++j)
            UTILSLIB::IOUtils::swap_floatp(&verts(i, j));

    // Read faces (nface x 3, int32)
    MatrixX3i faces(nface, 3);
    for (qint32 i = 0; i < nface; ++i) {
        for (qint32 j = 0; j < 3; ++j) {
            stream >> faces(i, j);
        }
    }

    file.close();

    // Vertices: transpose to nvert x 3 and convert from mm to meters
    verts.transposeInPlace();
    verts.array() *= 0.001f;

    //
    // Populate BEM surface
    //
    surf.np = nvert;
    surf.ntri = nface;
    surf.coord_frame = FIFFV_COORD_MRI;
    surf.rr = verts.block(0, 0, nvert, 3);
    surf.tris = faces;
    surf.id = id;
    surf.sigma = sigma;

    // Compute vertex normals
    surf.nn = Surface::compute_normals(surf.rr, surf.tris);

    // Shift vertices if requested
    if (shift != 0.0f) {
        shiftVertices(surf, shift / 1000.0f);   // Convert mm to meters
    }

    return true;
}

//=============================================================================================================

void SetupForwardModel::shiftVertices(MNEBemSurface& surf, float shift) const
{
    for (int k = 0; k < surf.np; ++k) {
        surf.rr(k, 0) += shift * surf.nn(k, 0);
        surf.rr(k, 1) += shift * surf.nn(k, 1);
        surf.rr(k, 2) += shift * surf.nn(k, 2);
    }

    // Recompute normals after shifting
    surf.nn = Surface::compute_normals(surf.rr, surf.tris);

    printf("Surface vertices shifted by %6.1f mm.\n", 1000.0f * shift);
}

//=============================================================================================================

bool SetupForwardModel::exportPntFile(const MNEBemSurface& surf,
                                      const QString& fileName) const
{
    //
    // Write ASCII .pnt file (mne_list_bem equivalent)
    //
    // Format: nvert on first line, then one "x y z" per line in mm
    //
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Could not open" << fileName << "for writing";
        return false;
    }

    QTextStream out(&file);
    out << surf.np << "\n";
    for (int k = 0; k < surf.np; ++k) {
        // Convert from meters to mm for output
        out << QString("%1 %2 %3\n")
            .arg(static_cast<double>(surf.rr(k, 0)) * 1000.0, 0, 'f', 4)
            .arg(static_cast<double>(surf.rr(k, 1)) * 1000.0, 0, 'f', 4)
            .arg(static_cast<double>(surf.rr(k, 2)) * 1000.0, 0, 'f', 4);
    }
    file.close();
    printf("Written %s\n", qPrintable(fileName));
    return true;
}

//=============================================================================================================

bool SetupForwardModel::exportSurfFile(const MNEBemSurface& surf,
                                       const QString& fileName) const
{
    //
    // Write FreeSurfer binary surface file (mne_list_bem --surf equivalent)
    //
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Could not open" << fileName << "for writing";
        return false;
    }

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::BigEndian);

    // Magic number for triangle surface: 0xFFFFFE
    ds << quint8(0xFF) << quint8(0xFF) << quint8(0xFE);

    // Two lines of text (comment + newline)
    QByteArray comment = "created by mne_setup_forward_model\n\n";
    file.write(comment);

    // Number of vertices and faces
    ds << qint32(surf.np) << qint32(surf.ntri);

    // Vertex coordinates (as float32 in mm — FreeSurfer convention)
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for (int k = 0; k < surf.np; ++k) {
        float x = surf.rr(k, 0) * 1000.0f;   // meters -> mm
        float y = surf.rr(k, 1) * 1000.0f;
        float z = surf.rr(k, 2) * 1000.0f;
        ds << x << y << z;
    }

    // Triangle vertex indices (as int32, 0-based)
    for (int k = 0; k < surf.ntri; ++k) {
        ds << qint32(surf.tris(k, 0))
           << qint32(surf.tris(k, 1))
           << qint32(surf.tris(k, 2));
    }

    file.close();
    printf("Written %s\n", qPrintable(fileName));
    return true;
}

//=============================================================================================================

bool SetupForwardModel::prepareBemSolution(const QString& bemFile,
                                           const QString& solFile) const
{
    //
    // Compute the BEM solution matrix using FwdBemModel
    // (mne_prepare_bem_model --bem $model --sol $model_sol --method linear)
    //
    // Steps:
    //   1. Load BEM surfaces from the geometry file
    //   2. Compute the linear collocation solution
    //   3. Save the BEM model with the solution to a new FIFF file
    //
    FwdBemModel* bemModel = nullptr;

    if (m_settings.homogeneous()) {
        bemModel = FwdBemModel::fwd_bem_load_homog_surface(const_cast<QString&>(bemFile));
    } else {
        bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(const_cast<QString&>(bemFile));
    }

    if (!bemModel) {
        qCritical() << "Could not load BEM surfaces from" << bemFile;
        return false;
    }

    printf("Computing the linear collocation solution...\n");

    int result = FwdBemModel::fwd_bem_compute_solution(bemModel, FWD_BEM_LINEAR_COLL);
    if (result != 0) {
        qCritical() << "BEM solution computation failed.";
        delete bemModel;
        return false;
    }

    printf("Solution computed.\n");

    //
    // Save the BEM solution:
    //   - Write FIFF file with BEM surfaces + solution matrix
    //   - Format matches what fwd_bem_load_solution expects:
    //     FIFFB_BEM block containing:
    //       - FIFF_BEM_APPROX (int: FIFFV_BEM_APPROX_LINEAR)
    //       - FIFF_BEM_POT_SOLUTION (float matrix: nsol x nsol)
    //       - FIFFB_BEM_SURF blocks for each surface
    //
    printf("Saving...\n");

    {
        QFile file(solFile);
        FiffStream::SPtr stream = FiffStream::start_file(file);

        stream->start_block(FIFFB_BEM);

        // Write BEM approximation method
        int approxMethod = FIFFV_BEM_APPROX_LINEAR;
        stream->write_int(FIFF_BEM_APPROX, &approxMethod);

        // Write BEM surfaces
        for (int k = 0; k < bemModel->nsurf; ++k) {
            stream->start_block(FIFFB_BEM_SURF);

            MneSurfaceOld* s = bemModel->surfs[k];

            // Surface ID
            int surfId = s->id;
            stream->write_int(FIFF_BEM_SURF_ID, &surfId);

            // Conductivity
            float sigma = bemModel->sigma[k];
            stream->write_float(FIFF_BEM_SIGMA, &sigma);

            // Coordinate frame
            int coordFrame = s->coord_frame;
            stream->write_int(FIFF_MNE_COORD_FRAME, &coordFrame);

            // Number of vertices and triangles
            stream->write_int(FIFF_BEM_SURF_NNODE, &s->np);
            stream->write_int(FIFF_BEM_SURF_NTRI, &s->ntri);

            // Vertex coordinates
            MatrixXf rr(s->np, 3);
            for (int v = 0; v < s->np; ++v) {
                rr(v, 0) = s->rr[v][0];
                rr(v, 1) = s->rr[v][1];
                rr(v, 2) = s->rr[v][2];
            }
            stream->write_float_matrix(FIFF_BEM_SURF_NODES, rr);

            // Triangle indices (convert to 1-based for FIFF)
            if (s->ntri > 0) {
                MatrixXi tris(s->ntri, 3);
                for (int t = 0; t < s->ntri; ++t) {
                    tris(t, 0) = s->tris[t].vert[0] + 1;
                    tris(t, 1) = s->tris[t].vert[1] + 1;
                    tris(t, 2) = s->tris[t].vert[2] + 1;
                }
                stream->write_int_matrix(FIFF_BEM_SURF_TRIANGLES, tris);
            }

            // Vertex normals
            MatrixXf nn(s->np, 3);
            for (int v = 0; v < s->np; ++v) {
                nn(v, 0) = s->nn[v][0];
                nn(v, 1) = s->nn[v][1];
                nn(v, 2) = s->nn[v][2];
            }
            stream->write_float_matrix(FIFF_BEM_SURF_NORMALS, nn);

            stream->end_block(FIFFB_BEM_SURF);
        }

        // Write the solution matrix
        if (bemModel->solution && bemModel->nsol > 0) {
            MatrixXf solMat(bemModel->nsol, bemModel->nsol);
            for (int i = 0; i < bemModel->nsol; ++i) {
                for (int j = 0; j < bemModel->nsol; ++j) {
                    solMat(i, j) = bemModel->solution[i][j];
                }
            }
            stream->write_float_matrix(FIFF_BEM_POT_SOLUTION, solMat);
        }

        stream->end_block(FIFFB_BEM);
        stream->end_file();
    }

    printf("Saved the result to %s\n", qPrintable(solFile));

    delete bemModel;
    return true;
}
