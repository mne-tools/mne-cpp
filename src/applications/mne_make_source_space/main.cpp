//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Matti Hamalainen. All rights reserved.
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
 * @brief    Implements mne_make_source_space application.
 *           Port of the original MNE-C mne_make_source_space by Matti Hamalainen.
 *
 *           Creates cortical source spaces by subsampling FreeSurfer surfaces
 *           using icosahedron/octahedron-based or spacing-based decimation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_sourcespace.h>

#include <fs/surface.h>
#include <fs/surfaceset.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "1.0"

//=============================================================================================================
/**
 * @brief Build an icosahedron and subdivide it to the given grade.
 *
 * Grade 0 = 12 vertices (base icosahedron)
 * Grade n = each triangle subdivided 4^n times
 *
 * For negative grade, use octahedron subdivision instead (oct grade = -ico_grade).
 *
 * @param[in]  grade   Subdivision grade (1-7 for ico, negative for oct).
 * @param[out] verts   Vertex positions on the unit sphere.
 * @return true on success.
 */
static bool makeIcosahedron(int grade, MatrixX3f &verts)
{
    // Golden ratio
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    // Base icosahedron vertices (12 vertices)
    QVector<Vector3f> icoVerts;
    icoVerts.reserve(12);
    icoVerts << Vector3f(-1,  phi, 0) << Vector3f( 1,  phi, 0)
             << Vector3f(-1, -phi, 0) << Vector3f( 1, -phi, 0)
             << Vector3f(0, -1,  phi) << Vector3f(0,  1,  phi)
             << Vector3f(0, -1, -phi) << Vector3f(0,  1, -phi)
             << Vector3f( phi, 0, -1) << Vector3f( phi, 0,  1)
             << Vector3f(-phi, 0, -1) << Vector3f(-phi, 0,  1);

    // Normalize to unit sphere
    for (auto &v : icoVerts)
        v.normalize();

    // Base icosahedron triangles (20 faces)
    QVector<Vector3i> icoFaces;
    icoFaces.reserve(20);
    icoFaces << Vector3i(0, 11, 5)  << Vector3i(0, 5, 1)   << Vector3i(0, 1, 7)
             << Vector3i(0, 7, 10)  << Vector3i(0, 10, 11)  << Vector3i(1, 5, 9)
             << Vector3i(5, 11, 4)  << Vector3i(11, 10, 2)  << Vector3i(10, 7, 6)
             << Vector3i(7, 1, 8)   << Vector3i(3, 9, 4)    << Vector3i(3, 4, 2)
             << Vector3i(3, 2, 6)   << Vector3i(3, 6, 8)    << Vector3i(3, 8, 9)
             << Vector3i(4, 9, 5)   << Vector3i(2, 4, 11)   << Vector3i(6, 2, 10)
             << Vector3i(8, 6, 7)   << Vector3i(9, 8, 1);

    // Subdivide
    for (int g = 0; g < std::abs(grade); ++g) {
        QVector<Vector3f> newVerts = icoVerts;
        QVector<Vector3i> newFaces;
        QMap<QPair<int,int>, int> edgeMidpoint;

        auto getMidpoint = [&](int v0, int v1) -> int {
            auto key = qMakePair(qMin(v0, v1), qMax(v0, v1));
            if (edgeMidpoint.contains(key))
                return edgeMidpoint[key];
            Vector3f mid = (newVerts[v0] + newVerts[v1]).normalized();
            int idx = newVerts.size();
            newVerts.append(mid);
            edgeMidpoint[key] = idx;
            return idx;
        };

        for (const auto &f : icoFaces) {
            int a = getMidpoint(f(0), f(1));
            int b = getMidpoint(f(1), f(2));
            int c = getMidpoint(f(2), f(0));

            newFaces << Vector3i(f(0), a, c)
                     << Vector3i(a, f(1), b)
                     << Vector3i(c, b, f(2))
                     << Vector3i(a, b, c);
        }

        icoVerts = newVerts;
        icoFaces = newFaces;
    }

    // Copy to output
    verts.resize(icoVerts.size(), 3);
    for (int i = 0; i < icoVerts.size(); ++i) {
        verts.row(i) = icoVerts[i].transpose();
    }

    printf("  Icosahedron grade %d: %d vertices\n", std::abs(grade), (int)verts.rows());
    return true;
}

//=============================================================================================================
/**
 * @brief Select source vertices on a surface by finding the nearest surface
 *        vertex to each icosahedron point.
 *
 * @param[in]  surfVerts   The high-resolution surface vertices (np x 3).
 * @param[in]  icoVerts    Icosahedron vertices on unit sphere (nico x 3).
 * @param[out] inuse       Binary vector (np): 1 = selected, 0 = not.
 * @param[out] vertno      Indices of the selected vertices.
 * @return Number of selected vertices.
 */
static int selectVerticesIco(const MatrixX3f &surfVerts,
                             const MatrixX3f &icoVerts,
                             VectorXi &inuse,
                             VectorXi &vertno)
{
    int np = surfVerts.rows();
    int nico = icoVerts.rows();

    // Normalize surface vertices to the unit sphere
    MatrixX3f surfNorm(np, 3);
    for (int i = 0; i < np; ++i) {
        surfNorm.row(i) = surfVerts.row(i).normalized();
    }

    inuse = VectorXi::Zero(np);

    // For each icosahedron point, find the closest surface vertex
    for (int k = 0; k < nico; ++k) {
        float minDist = std::numeric_limits<float>::max();
        int minIdx = 0;

        for (int i = 0; i < np; ++i) {
            float dist = (surfNorm.row(i) - icoVerts.row(k)).squaredNorm();
            if (dist < minDist) {
                minDist = dist;
                minIdx = i;
            }
        }
        inuse(minIdx) = 1;
    }

    // Collect selected vertex indices
    int nuse = inuse.sum();
    vertno.resize(nuse);
    int idx = 0;
    for (int i = 0; i < np; ++i) {
        if (inuse(i)) {
            vertno(idx++) = i;
        }
    }

    return nuse;
}

//=============================================================================================================
/**
 * @brief Select source vertices by approximate spacing in mm.
 *
 * Selects a subset of surface vertices such that the minimum distance
 * between any two selected vertices is approximately the given spacing.
 *
 * @param[in]  surfVerts   Surface vertex positions (np x 3).
 * @param[in]  spacing     Desired spacing in mm.
 * @param[out] inuse       Binary vector (np): 1 = selected, 0 = not.
 * @param[out] vertno      Indices of the selected vertices.
 * @return Number of selected vertices.
 */
static int selectVerticesSpacing(const MatrixX3f &surfVerts,
                                 float spacing,
                                 VectorXi &inuse,
                                 VectorXi &vertno)
{
    int np = surfVerts.rows();
    float spacingM = spacing / 1000.0f;  // mm to m
    float spacingSq = spacingM * spacingM;

    inuse = VectorXi::Zero(np);

    // Greedy selection: iterate through vertices and select if far enough
    // from all previously selected vertices
    QVector<int> selected;
    selected.reserve(np / 10);

    for (int i = 0; i < np; ++i) {
        bool tooClose = false;
        for (int j = 0; j < selected.size(); ++j) {
            float dist = (surfVerts.row(i) - surfVerts.row(selected[j])).squaredNorm();
            if (dist < spacingSq) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            selected.append(i);
            inuse(i) = 1;
        }
    }

    int nuse = selected.size();
    vertno.resize(nuse);
    for (int i = 0; i < nuse; ++i) {
        vertno(i) = selected[i];
    }

    return nuse;
}

//=============================================================================================================
/**
 * @brief Build a MNEHemisphere from a FreeSurfer surface with vertex selection.
 *
 * @param[in]  surf     The FreeSurfer surface.
 * @param[in]  inuse    Binary vector indicating selected vertices.
 * @param[in]  vertno   Indices of selected vertices.
 * @param[in]  hemiId   Hemisphere ID (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI).
 * @return The constructed MNEHemisphere.
 */
static MNEHemisphere buildHemisphere(const Surface &surf,
                                     const VectorXi &inuse,
                                     const VectorXi &vertno,
                                     int hemiId)
{
    MNEHemisphere hemi;

    hemi.id = hemiId;
    hemi.np = surf.rr().rows();
    hemi.ntri = surf.tris().rows();
    hemi.coord_frame = FIFFV_COORD_MRI;
    hemi.type = 1;  // Surface type

    // Copy vertex positions and normals
    hemi.rr = surf.rr();
    hemi.nn = surf.nn();
    hemi.tris = surf.tris();

    // Selection info
    hemi.nuse = vertno.size();
    hemi.inuse = inuse;
    hemi.vertno = vertno;

    // Build use_tris (triangles that use only selected vertices)
    QVector<int> useTrisIdx;
    for (int t = 0; t < hemi.ntri; ++t) {
        if (inuse(hemi.tris(t, 0)) &&
            inuse(hemi.tris(t, 1)) &&
            inuse(hemi.tris(t, 2))) {
            useTrisIdx.append(t);
        }
    }
    hemi.nuse_tri = useTrisIdx.size();
    hemi.use_tris.resize(hemi.nuse_tri, 3);
    for (int i = 0; i < hemi.nuse_tri; ++i) {
        hemi.use_tris.row(i) = hemi.tris.row(useTrisIdx[i]);
    }

    // Add geometry information (normals, triangle centers, etc.)
    hemi.add_geometry_info();

    return hemi;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_make_source_space application.
 * This is a port of the original MNE-C mne_make_source_space by Matti Hamalainen.
 *
 * It reads FreeSurfer surface files for left and right hemispheres, subsamples
 * them using icosahedron-based or spacing-based decimation, and writes the
 * resulting source space to a FIFF file.
 *
 * @param[in] argc  (argument count)
 * @param[in] argv  (argument vector)
 * @return exit code (0 on success).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_source_space");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Create cortical source spaces from FreeSurfer surfaces.\n"
        "Port of the original MNE-C mne_make_source_space by Matti Hamalainen.\n\n"
        "Reads FreeSurfer surface meshes for left and right hemispheres,\n"
        "subsamples them using icosahedron or spacing-based decimation,\n"
        "and writes the resulting source space to a FIFF file.\n\n"
        "Requires --surf and a decimation method (--ico, --oct, --spacing, or --all).\n"
        "Subject defaults to $SUBJECT environment variable if --subject is not given."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    // --subject: Subject name
    QCommandLineOption subjectOpt(QStringList() << "subject",
        "Name of the FreeSurfer subject.", "name");
    parser.addOption(subjectOpt);

    // --subjects_dir: Subjects directory (default: $SUBJECTS_DIR)
    QCommandLineOption subjectsDirOpt(QStringList() << "subjects_dir",
        "FreeSurfer subjects directory (default: $SUBJECTS_DIR).", "dir");
    parser.addOption(subjectsDirOpt);

    // --surf: Surface name(s) (required, matching SVN MNE-C)
    QCommandLineOption surfOpt(QStringList() << "surf",
        "Surface name to use (e.g., white, pial). "
        "For multiple surfaces, separate with colons (e.g., white:pial).",
        "names");
    parser.addOption(surfOpt);

    // --ico: Icosahedron subdivision grade
    QCommandLineOption icoOpt(QStringList() << "ico",
        "Icosahedron subdivision grade for downsampling (1-7).\n"
        "Grade 1 = 42 vertices, 2 = 162, 3 = 642, 4 = 2562, 5 = 10242, 6 = 40962, 7 = 163842.",
        "grade");
    parser.addOption(icoOpt);

    // --oct: Octahedron subdivision grade
    QCommandLineOption octOpt(QStringList() << "oct",
        "Octahedron subdivision grade (stored as negative ico grade internally).",
        "grade");
    parser.addOption(octOpt);

    // --spacing: Approximate spacing in mm
    QCommandLineOption spacingOpt(QStringList() << "spacing",
        "Approximate source space spacing in mm (alternative to --ico/--oct).",
        "dist");
    parser.addOption(spacingOpt);

    // --all: Use all surface vertices
    QCommandLineOption allOpt(QStringList() << "all",
        "Use all surface vertices (no decimation).");
    parser.addOption(allOpt);

    // --src: Output file name
    QCommandLineOption srcOpt(QStringList() << "src",
        "Output source space file (default: <subject>-<ico/spacing>-src.fif).",
        "file");
    parser.addOption(srcOpt);

    parser.process(app);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.isSet(subjectOpt)) {
        // Fall back to $SUBJECT environment variable (matching SVN MNE-C)
        QString envSubject = qEnvironmentVariable("SUBJECT");
        if (envSubject.isEmpty()) {
            qCritical() << "Error: --subject option is required (or set $SUBJECT).";
            parser.showHelp(1);
        }
        printf("Using subject from $SUBJECT: %s\n", envSubject.toUtf8().constData());
    }

    QString subject = parser.isSet(subjectOpt) ? parser.value(subjectOpt)
                                               : qEnvironmentVariable("SUBJECT");
    QString subjectsDir;
    if (parser.isSet(subjectsDirOpt)) {
        subjectsDir = parser.value(subjectsDirOpt);
    } else {
        subjectsDir = qEnvironmentVariable("SUBJECTS_DIR");
        if (subjectsDir.isEmpty()) {
            qCritical() << "Error: SUBJECTS_DIR not set. Use --subjects_dir.";
            return 1;
        }
    }

    // --surf is required (matching SVN MNE-C)
    if (!parser.isSet(surfOpt)) {
        qCritical() << "Error: --surf option is required.";
        parser.showHelp(1);
    }
    QString surfName = parser.value(surfOpt);

    // Determine decimation method (must be explicitly specified)
    enum DecimMethod { ICO, SPACING, ALL } decimMethod;
    int icoGrade = 0;
    float spacing = 0.0f;

    if (parser.isSet(allOpt)) {
        decimMethod = ALL;
    } else if (parser.isSet(spacingOpt)) {
        decimMethod = SPACING;
        spacing = parser.value(spacingOpt).toFloat();
        if (spacing <= 0.0f) {
            qCritical() << "Error: --spacing must be a positive number.";
            return 1;
        }
    } else if (parser.isSet(icoOpt) || parser.isSet(octOpt)) {
        decimMethod = ICO;
        if (parser.isSet(icoOpt)) {
            icoGrade = parser.value(icoOpt).toInt();
        } else {
            icoGrade = parser.value(octOpt).toInt();
        }
        if (icoGrade < 1 || icoGrade > 7) {
            qCritical() << "Error: --ico/--oct grade should be between 1 and 7.";
            return 1;
        }
    } else {
        qCritical() << "Error: A decimation method is required (--ico, --oct, --spacing, or --all).";
        parser.showHelp(1);
    }

    // Determine output file name
    QString srcName;
    if (parser.isSet(srcOpt)) {
        srcName = parser.value(srcOpt);
    } else {
        QString tag;
        if (decimMethod == ICO)
            tag = QString("ico-%1").arg(icoGrade);
        else if (decimMethod == SPACING)
            tag = QString("spacing-%1").arg((int)spacing);
        else
            tag = "all";

        srcName = subjectsDir + "/" + subject + "/bem/" + subject + "-" + tag + "-src.fif";
    }

    //=========================================================================================================
    // Build icosahedron for decimation (if needed)
    //=========================================================================================================

    MatrixX3f icoVerts;
    if (decimMethod == ICO) {
        printf("\nBuilding icosahedron with grade %d for decimation...\n", icoGrade);
        if (!makeIcosahedron(icoGrade, icoVerts)) {
            qCritical() << "Error: Failed to build icosahedron.";
            return 1;
        }
    }

    //=========================================================================================================
    // Process each hemisphere
    //=========================================================================================================

    QStringList hemiNames = {"lh", "rh"};
    int hemiIds[2] = {FIFFV_MNE_SURF_LEFT_HEMI, FIFFV_MNE_SURF_RIGHT_HEMI};

    QVector<MNEHemisphere> hemispheres;

    for (int h = 0; h < 2; ++h) {
        printf("\n========================================\n");
        printf("Processing %s hemisphere...\n", hemiNames[h].toUtf8().constData());

        // Read FreeSurfer surface
        QString surfPath = subjectsDir + "/" + subject + "/surf/" +
                           hemiNames[h] + "." + surfName;

        printf("Reading surface from %s...\n", surfPath.toUtf8().constData());

        Surface surf;
        if (!Surface::read(surfPath, surf)) {
            qCritical() << "Error: Could not read surface" << surfPath;
            return 1;
        }

        printf("  Surface: %d vertices, %d triangles\n",
               (int)surf.rr().rows(), (int)surf.tris().rows());

        // Select source vertices
        VectorXi inuse;
        VectorXi vertno;
        int nuse;

        if (decimMethod == ALL) {
            printf("  Using all vertices.\n");
            int np = surf.rr().rows();
            inuse = VectorXi::Ones(np);
            vertno.resize(np);
            for (int i = 0; i < np; ++i) vertno(i) = i;
            nuse = np;
        } else if (decimMethod == ICO) {
            printf("  Decimating with icosahedron grade %d...\n", icoGrade);
            nuse = selectVerticesIco(surf.rr(), icoVerts, inuse, vertno);
        } else {
            printf("  Decimating with spacing %.1f mm...\n", spacing);
            nuse = selectVerticesSpacing(surf.rr(), spacing, inuse, vertno);
        }

        printf("  Selected %d source locations.\n", nuse);

        if (nuse == 0) {
            qCritical() << "Error: No vertices selected for" << hemiNames[h];
            return 1;
        }

        // Build hemisphere
        MNEHemisphere hemi = buildHemisphere(surf, inuse, vertno, hemiIds[h]);
        hemispheres.append(hemi);
    }

    //=========================================================================================================
    // Write source space
    //=========================================================================================================

    printf("\n========================================\n");
    printf("Writing source space to %s...\n", srcName.toUtf8().constData());

    // Ensure output directory exists
    QFileInfo fi(srcName);
    QDir().mkpath(fi.path());

    QFile srcFile(srcName);
    if (!srcFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Error: Could not open output file" << srcName;
        return 1;
    }

    FiffStream::SPtr pStream = FiffStream::start_file(srcFile);
    pStream->start_block(FIFFB_MNE);
    for (int h = 0; h < hemispheres.size(); ++h) {
        hemispheres[h].writeToStream(pStream.data());
    }
    pStream->end_block(FIFFB_MNE);
    pStream->end_file();
    srcFile.close();

    // Summary
    printf("\nSource space created successfully:\n");
    for (int h = 0; h < hemispheres.size(); ++h) {
        printf("  %s hemisphere: %d of %d vertices selected",
               h == 0 ? "Left" : "Right",
               hemispheres[h].nuse, hemispheres[h].np);
        if (hemispheres[h].nuse_tri > 0) {
            printf(", %d use triangles", hemispheres[h].nuse_tri);
        }
        printf("\n");
    }
    printf("\nOutput: %s\n", srcName.toUtf8().constData());
    printf("\nYou can now use mne_forward_solution to compute forward solutions\n");
    printf("using this source space.\n\n");

    return 0;
}
