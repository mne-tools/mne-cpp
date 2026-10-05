//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    FreeSurfer library: surfaces, curvature, parcellations, labels and an atlas volume.
 *
 * Reads the sample subject of the MNE-CPP test data and compares counts and
 * values with nibabel and MNE-Python 1.11 (read_geometry, read_morph_data,
 * read_annot, read_label, read_labels_from_annot). The atlas lookup runs on a
 * small MGH volume written by the example. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_atlas_lookup.h>
#include <fs/fs_colortable.h>
#include <fs/fs_label.h>
#include <fs/fs_label_utils.h>
#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <functional>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================
/**
 * Writes a 4x4x4 uchar MGH volume with 2 mm voxels whose voxel (0,0,0) sits at RAS (-4,-4,-4) mm.
 */
bool writeAtlas(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);
    out << qint32(1) << qint32(4) << qint32(4) << qint32(4) << qint32(1) << qint32(0) << qint32(0) << qint16(1);
    out << 2.0f << 2.0f << 2.0f;                                                 // voxel size
    out << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f; // direction cosines
    out << 0.0f << 0.0f << 0.0f;                                                 // centre RAS
    file.write(QByteArray(284 - 90, '\0'));
    QByteArray voxels(64, '\0');
    voxels[0] = 17;                   // (0,0,0): Left-Hippocampus
    voxels[3 + 4 * (2 + 4 * 1)] = 53; // (3,2,1): Right-Hippocampus
    file.write(voxels);
    return true;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption subjectsOption("subjectsDir", "FreeSurfer <dir>.", "dir",
                                      QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects");
    parser.addOption(subjectsOption);
    parser.process(app);
    const QString subjectsDir = parser.value(subjectsOption);
    bool ok = true;

    //! [fs_surface_read]
    FsSurface white("sample", 0, "white", subjectsDir); // lh = 0, rh = 1; curvature is loaded alongside
    const MatrixX3f& rr = white.rr();                   // vertices (metres)
    const MatrixX3i& tris = white.tris();
    const VectorXf& curv = white.curv();
    //! [fs_surface_read]
    // nibabel.freesurfer.read_geometry / read_morph_data: 155407 vertices, 310810 triangles, curv[0] = -0.205397
    ok &= expect(rr.rows() == 155407 && tris.rows() == 310810 && std::fabs(curv(0) + 0.205397367477417f) < 1e-6f && (rr.row(0) * 1000.0f - RowVector3f(-20.95193099975586f, -86.27677917480469f, 13.240333557128906f)).norm() < 1e-3f,
                 QString("lh.white: %1 vertices, %2 triangles, first vertex and curvature = nibabel").arg(rr.rows()).arg(tris.rows()));

    //! [fs_surface_set_read]
    FsSurfaceSet both("sample", 2, "white", subjectsDir); // 2 = both hemispheres
    //! [fs_surface_set_read]
    ok &= expect(both.size() == 2 && both[0].rr().rows() == 155407, QString("FsSurfaceSet holds %1 hemispheres").arg(both.size()));

    //! [fs_annotation_read]
    FsAnnotation aparc("sample", 0, "aparc", subjectsDir);
    FsColortable& colors = aparc.getColortable();
    QList<FsLabel> regions;
    QList<RowVector4i> regionColors;
    aparc.toLabels(white, regions, regionColors);
    //! [fs_annotation_read]
    int precentral = -1;
    for (int i = 0; i < regions.size(); ++i) {
        if (regions[i].name == "precentral-lh") {
            precentral = i;
        }
    }
    // nibabel.freesurfer.read_annot: 36 table entries; mne.read_labels_from_annot: 34 labels, precentral-lh 9505 vertices
    ok &= expect(colors.getNames().size() == 36 && regions.size() == 34 && precentral >= 0 && regions[precentral].vertices.size() == 9505,
                 QString("aparc: %1 colour-table entries, precentral %2 vertices (9505)")
                     .arg(colors.getNames().size())
                     .arg(precentral >= 0 ? regions[precentral].vertices.size() : -1));

    //! [fs_annotation_set_read]
    FsAnnotationSet parcellation("sample", 2, "aparc", subjectsDir);
    //! [fs_annotation_set_read]
    ok &= expect(parcellation.size() == 2, "FsAnnotationSet holds both hemispheres");

    //! [fs_label_read]
    FsLabel v1;
    FsLabel::read(subjectsDir + "/sample/label/lh.V1.label", v1);
    //! [fs_label_read]
    // File order (mne.read_label sorts): first vertex 1093 at (-15.435, -83.078, -1.042) mm
    ok &= expect(v1.vertices.size() == 5638 && v1.vertices(0) == 1093 && (v1.pos.row(0) - RowVector3f(-0.015435f, -0.083078f, -0.001042f)).norm() < 1e-6f,
                 QString("lh.V1.label: %1 vertices = mne.read_label").arg(v1.vertices.size()));

    //! [fs_label_utils_usage]
    const FsLabel grown = FsLabelUtils::growLabel(v1, white, 1); // add the one-ring neighbours
    const QList<FsLabel> parts = FsLabelUtils::splitLabel(v1, white);
    //! [fs_label_utils_usage]
    // Closed form on the mesh graph (scipy.sparse.csgraph): one ring adds 447 vertices; components of 5635, 2 and 1
    QList<int> sizes;
    for (const FsLabel& part : parts) {
        sizes.append(part.vertices.size());
    }
    std::sort(sizes.begin(), sizes.end(), std::greater<int>());
    ok &= expect(grown.vertices.size() - v1.vertices.size() == 447 && sizes == QList<int>({5635, 2, 1}),
                 QString("growLabel adds %1 vertices (447); splitLabel parts %2").arg(grown.vertices.size() - v1.vertices.size()).arg(sizes.size()));

    //! [fs_atlas_lookup_usage]
    QTemporaryDir dir;
    const QString atlasPath = dir.filePath("atlas.mgh"); // normally aparc+aseg.mgz
    writeAtlas(atlasPath);
    FsAtlasLookup atlas;
    atlas.load(atlasPath);
    const QStringList regionsAt = atlas.labelsForPositions({Vector3f(-4.0f, -4.0f, -4.0f), Vector3f(2.0f, 0.0f, -2.0f), Vector3f(0.0f, 0.0f, 0.0f)});
    //! [fs_atlas_lookup_usage]
    // nibabel: inv(affine) maps (-4,-4,-4) to voxel (0,0,0) and (2,0,-2) to voxel (3,2,1)
    ok &= expect(atlas.isLoaded() && regionsAt == QStringList({"Left-Hippocampus", "Right-Hippocampus", "Unknown"}),
                 QString("atlas lookup: %1").arg(regionsAt.join(", ")));

    qInfo() << (ok ? "All fs checks passed." : "fs checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
