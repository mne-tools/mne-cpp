//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    MRI library: MGH/MGZ and NIfTI volumes, voxel/RAS transforms, slicing and COR sets.
 *
 * data/gradient.mgz and data/gradient.nii.gz hold the same 6x5x4 volume
 * (value = i + 10 j + 50 k) with a rotated, anisotropic voxel-to-RAS affine,
 * written by nibabel. The example checks dimensions, the affine and voxel
 * values against nibabel, slices the volume, and writes a COR set and COR.fif.
 * Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri/mri_cor_fif_io.h>
#include <mri/mri_cor_io.h>
#include <mri/mri_mgh_io.h>
#include <mri/mri_nifti_io.h>
#include <mri/mri_slicer.h>
#include <mri/mri_vol_data.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace FIFFLIB;
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

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption dataOption("dataDir", "Directory with gradient.mgz and gradient.nii.gz <dir>.", "dir", MNE_EX_MRI_DATA_DIR);
    parser.addOption(dataOption);
    parser.process(app);
    const QString dataDir = parser.value(dataOption);
    bool ok = true;

    // nibabel affine of the fixture, in mm
    Matrix4f nibabelAffine;
    nibabelAffine << -1.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 1.5f, -2.0f, 0.0f, -2.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f;

    //! [mri_mgh_io_read]
    MriVolData mgh;
    QVector<FiffCoordTrans> extraTransforms; // e.g. Talairach, if the footer names one
    MriMghIO::read(dataDir + "/gradient.mgz", mgh, extraTransforms);
    const Matrix4f vox2ras = mgh.computeVox2Ras();        // scanner RAS, metres
    const QVector<float> voxels = mgh.voxelDataAsFloat(); // x fastest
    //! [mri_mgh_io_read]
    Matrix4f vox2rasMm = vox2ras;
    vox2rasMm.block<3, 4>(0, 0) *= 1000.0f;
    ok &= expect(mgh.dims() == QVector<int>({6, 5, 4}) && (vox2rasMm - nibabelAffine).cwiseAbs().maxCoeff() < 1e-4f && voxels.size() == 120 && voxels[2 + 6 * (3 + 5 * 1)] == 82.0f,
                 "MGZ: 6x5x4 voxels, vox2ras and voxel (2,3,1) = 82 match nibabel");

    //! [mri_vol_data_read]
    MriVolData nifti;
    nifti.read(dataDir + "/gradient.nii.gz"); // the suffix selects the NIfTI reader
    //! [mri_vol_data_read]
    Matrix4f niftiMm = nifti.computeVox2Ras();
    niftiMm.block<3, 4>(0, 0) *= 1000.0f;
    ok &= expect(nifti.isValid() && nifti.dims() == mgh.dims() && nifti.voxelDataAsFloat() == voxels && (niftiMm - nibabelAffine).cwiseAbs().maxCoeff() < 1e-4f,
                 "NIfTI: same voxels and affine as the MGZ");

    //! [mri_nifti_io_read]
    MriVolData direct;
    MriNiftiIO::read(dataDir + "/gradient.nii.gz", direct);
    //! [mri_nifti_io_read]
    ok &= expect(direct.voxelDataAsFloat() == voxels, "MriNiftiIO::read equals MriVolData::read");

    //! [mri_slicer_usage]
    const Vector3f ras(0.001f, -0.0005f, -0.002f); // metres; voxel (2,3,1)
    const Vector3i voxel = MriSlicer::rasToVoxel(vox2ras, ras);
    const QVector<MriSliceImage> planes = MriSlicer::extractOrthogonal(voxels, mgh.dims(), vox2ras, ras); // axial, coronal, sagittal
    //! [mri_slicer_usage]
    ok &= expect(voxel == Vector3i(2, 3, 1) && (MriSlicer::voxelToRas(vox2ras, voxel) - ras).norm() < 1e-7f && planes.size() == 3,
                 QString("RAS (1, -0.5, -2) mm -> voxel (%1, %2, %3); three orthogonal planes").arg(voxel(0)).arg(voxel(1)).arg(voxel(2)));

    // A COR set is 256 coronal 256x256 byte slices
    QTemporaryDir dir;
    const QString corDir = dir.filePath("T1");
    QDir().mkpath(corDir);
    for (int k = 0; k < 256; ++k) {
        QFile slice(QString("%1/COR-%2").arg(corDir).arg(k + 1, 3, 10, QChar('0')));
        if (!slice.open(QIODevice::WriteOnly) || slice.write(QByteArray(256 * 256, static_cast<char>(k))) != 256 * 256) {
            qCritical() << "cannot write" << slice.fileName();
            return EXIT_FAILURE;
        }
    }

    //! [mri_cor_io_read]
    QVector<MriSlice> corSlices;
    MriCorIO::read(corDir, corSlices);
    //! [mri_cor_io_read]
    ok &= expect(corSlices.size() == 256 && corSlices[7].pixels.size() == 256 * 256 && corSlices[7].pixels[0] == 7,
                 QString("COR set: %1 slices of 256x256, slice 7 holds 7").arg(corSlices.size()));

    //! [mri_cor_fif_io_write]
    const QString corFif = dir.filePath("COR.fif");
    MriCorFifIO::write(corFif, corSlices, extraTransforms);
    //! [mri_cor_fif_io_write]
    ok &= expect(QFile(corFif).size() > 256 * 256 * 256, QString("COR.fif written (%1 bytes)").arg(QFile(corFif).size()));

    qInfo() << (ok ? "All mri checks passed." : "mri checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
