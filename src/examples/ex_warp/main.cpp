//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     May 2026
 * @brief    Example demonstrating thin-plate-spline (TPS) warping with UTILSLIB::Warp.
 *
 * This example shows how to non-rigidly register one 3-D geometry onto another
 * using a set of landmark correspondences. It defines a small set of source
 * landmarks (the corners of a unit cube), a matching set of destination
 * landmarks (an affine stretch plus a non-linear bump), warps a regular grid of
 * source vertices onto the destination geometry, and verifies that the warp maps
 * the source landmarks exactly onto the destination landmarks - the defining
 * property of Bookstein's thin-plate spline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math/warp.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main starts the warp example.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the
 *                  command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects.
 *
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //
    // Source landmarks: the eight corners of a unit cube.
    //
    MatrixXf sourceLm(8, 3);
    sourceLm << 0.f, 0.f, 0.f,
                1.f, 0.f, 0.f,
                0.f, 1.f, 0.f,
                1.f, 1.f, 0.f,
                0.f, 0.f, 1.f,
                1.f, 0.f, 1.f,
                0.f, 1.f, 1.f,
                1.f, 1.f, 1.f;

    //
    // Destination landmarks: an affine stretch (x2 along x, x1.5 along y) plus a
    // non-linear bump that lifts the cube's top face. This is the kind of
    // template-to-subject deformation TPS is designed to capture.
    //
    MatrixXf destLm = sourceLm;
    destLm.col(0) *= 2.0f;
    destLm.col(1) *= 1.5f;
    for(int i = 0; i < destLm.rows(); ++i) {
        destLm(i, 2) += 0.25f * sourceLm(i, 2);     // lift proportional to height
    }

    //
    // Source vertices: a regular 3x3x3 grid spanning the source cube. These stand
    // in for the vertices of a template mesh (e.g. a scalp or cortex surface).
    //
    const int iN = 3;
    MatrixXf sourceVert(iN * iN * iN, 3);
    int iRow = 0;
    for(int ix = 0; ix < iN; ++ix) {
        for(int iy = 0; iy < iN; ++iy) {
            for(int iz = 0; iz < iN; ++iz) {
                sourceVert.row(iRow++) << ix / float(iN - 1),
                                          iy / float(iN - 1),
                                          iz / float(iN - 1);
            }
        }
    }

    //
    // Fit the thin-plate spline from the landmark correspondences and apply it to
    // the source vertices in one call.
    //
    Warp warp;
    MatrixXf warpedVert = warp.calculate(sourceLm, destLm, sourceVert);

    //
    // Verify the defining TPS property: the source landmarks map exactly onto the
    // destination landmarks. We warp the landmarks themselves and compare.
    //
    MatrixXf warpedLm = warp.calculate(sourceLm, destLm, sourceLm);
    const float fMaxLmError = (warpedLm - destLm).cwiseAbs().maxCoeff();

    qInfo() << "Thin-plate-spline warp example";
    qInfo() << "  source landmarks :" << sourceLm.rows();
    qInfo() << "  warped vertices  :" << warpedVert.rows();
    qInfo() << "  max landmark interpolation error :" << fMaxLmError;

    if(fMaxLmError < 1e-3f) {
        qInfo() << "  -> landmarks reproduced exactly (TPS interpolation property holds).";
    } else {
        qWarning() << "  -> unexpected landmark error; the warp did not interpolate the landmarks.";
        return 1;
    }

    qInfo() << "First warped vertex :"
            << warpedVert(0, 0) << warpedVert(0, 1) << warpedVert(0, 2);
    qInfo() << "Last warped vertex  :"
            << warpedVert(warpedVert.rows() - 1, 0)
            << warpedVert(warpedVert.rows() - 1, 1)
            << warpedVert(warpedVert.rows() - 1, 2);

    return 0;
}
