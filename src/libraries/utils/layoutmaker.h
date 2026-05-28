//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file layoutmaker.h
 * @since 2022
 * @date  April 2026
 * @brief Generator that projects 3-D electrode positions onto a 2-D @c .lout topographic layout via least-squares sphere fitting.
 *
 * @ref UTILSLIB::LayoutMaker is the counterpart of
 * @ref UTILSLIB::LayoutLoader: given the 3-D coordinates of an
 * EEG cap (typically captured with the Polhemus digitizer or
 * loaded from a vendor @c .elc file) it produces the 2-D
 * channel layout file used by every topographic widget in
 * mne-cpp — butterfly plots, sensor maps, time-frequency
 * heatmaps in DISPLIB and DISP3DLIB.
 *
 * The projection is done by first fitting a sphere to the
 * supplied head-frame points (helper struct @ref fitUserRec
 * carries the working set for the non-linear fit), then
 * applying an azimuthal projection of every electrode onto
 * the tangent plane at the sphere apex. Optional mirroring
 * flags exist for caps whose left/right convention differs
 * from MNE's defaults.
 */

#ifndef LAYOUTMAKER_H
#define LAYOUTMAKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <vector>
#include <string>
#include <fstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QList>
#include <QStringList>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

/** @brief Workspace for sphere-fitting used by the layout maker, holding 3-D point coordinates and fit parameters. */
struct fitUserRec {
  Eigen::MatrixXf rr;
  int   np;
  int   report;
};
using fitUser = fitUserRec*;

//=============================================================================================================
/**
 * Make layout files from given 3D points
 *
 * @brief Make layout files from given 3D points.
 */
class UTILSSHARED_EXPORT LayoutMaker
{
public:

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] inputPoints       The input points in 3D space.
     * @param[in, out] outputPoints     The output layout points in 2D space.
     * @param[in] names             The channel names.
     * @param[in] outFile           The outout file.
     * @param[in] do_fit            The flag whether to do a sphere fitting.
     * @param[in] prad.
     * @param[in] w.
     * @param[in] h.
     * @param[in] writeFile         The flag whether to write to file.
     * @param[in] mirrorXAxis       Mirror points at x axis.
     * @param[in] mirrorYAxis       Mirror points at y axis.
     *
     * @return true if making layout was successful, false otherwise.
     */
    static bool makeLayout(const QList<QVector<float> > &inputPoints,
                           QList<QVector<float> > &outputPoints,
                           const QStringList &names,
                           QFile &outFile,
                           bool do_fit,
                           float prad,
                           float w,
                           float h,
                           bool writeFile = false,
                           bool mirrorXAxis = false,
                           bool mirrorYAxis = false);

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] inputPoints       The input points in 3D space.
     * @param[in, out] outputPoints     The output layout points in 2D space.
     * @param[in] names             The channel names.
     * @param[in] outFile           The outout file.
     * @param[in] do_fit            The flag whether to do a sphere fitting.
     * @param[in] prad.
     * @param[in] w.
     * @param[in] h.
     * @param[in] writeFile         The flag whether to write to file.
     * @param[in] mirrorXAxis       Mirror points at x axis.
     * @param[in] mirrorYAxis       Mirror points at y axis.
     *
     * @return true if making layout was successful, false otherwise.
     */
    static bool makeLayout(const std::vector<std::vector<float> > &inputPoints,
                           std::vector<std::vector<float> > &outputPoints,
                           const std::vector<std::string> &names,
                           const std::string& outFilePath,
                           bool do_fit,
                           float prad,
                           float w,
                           float h,
                           bool writeFile = false,
                           bool mirrorXAxis = false,
                           bool mirrorYAxis = false);

private:
    static void sphere_coord(float x,
                      float y,
                      float z,
                      float *r,
                      float *theta,
                      float *phi);
};
} //NAMESPACE

#endif // LAYOUTMAKER_H
