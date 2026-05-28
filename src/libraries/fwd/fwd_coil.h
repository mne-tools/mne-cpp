//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_coil.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Single MEG sensor coil or EEG electrode described by a set of weighted integration points in its own coordinate frame.
 *
 * In the Biot-Savart formulation used for MEG, the signal a coil records
 * is the surface integral of the dipole's magnetic field over the coil
 * area projected along the coil normal. The CTF / Elekta coil-definition
 * file approximates that integral as a weighted sum at a handful of
 * integration points; each FwdCoil stores those points as @c rmag
 * (positions), @c cosmag (orientations) and @c w (Gauss-Legendre or
 * trapezoidal weights). A magnetometer reduces to a single point; an
 * axial gradiometer uses two opposing loops with weights ±1; a planar
 * gradiometer uses four points encoding a finite-difference baseline.
 *
 * For EEG, FwdCoil collapses to a single point with @c cosmag = (0,0,0)
 * — the electrode is a potential probe, not a field probe — and @c w
 * carries the reference-electrode subtraction coefficient. The class is
 * a direct refactor of the MNE-C @c fwdCoilRec struct with raw
 * C-pointers replaced by Eigen matrices.
 */

#ifndef FWD_COIL_H
#define FWD_COIL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// COIL TYPE AND ACCURACY CONSTANTS
//=============================================================================================================

constexpr int FWD_COIL_UNKNOWN      = 0;

constexpr int FWD_COILC_UNKNOWN     = 0;
constexpr int FWD_COILC_EEG         = 1000;
constexpr int FWD_COILC_MAG         = 1;
constexpr int FWD_COILC_AXIAL_GRAD  = 2;
constexpr int FWD_COILC_PLANAR_GRAD = 3;
constexpr int FWD_COILC_AXIAL_GRAD2 = 4;

constexpr int FWD_COIL_ACCURACY_POINT    = 0;
constexpr int FWD_COIL_ACCURACY_NORMAL   = 1;
constexpr int FWD_COIL_ACCURACY_ACCURATE = 2;

inline constexpr bool FWD_IS_MEG_COIL(int x) { return (x != FWD_COILC_EEG && x != FWD_COILC_UNKNOWN); }

//=============================================================================================================
/**
 * Implements FwdCoil (replaces @c fwdCoil / @c fwdCoilRec from MNE-C @c fwd_types.h).
 *
 * @brief Single MEG sensor coil or EEG electrode — stores the coil-local frame and the @c (r_mag, cos_mag, w) integration-point triples that approximate the Biot-Savart surface integral over the coil area.
 */
class FWDSHARED_EXPORT FwdCoil
{
public:
    typedef std::unique_ptr<FwdCoil> UPtr;                /**< Unique pointer type for FwdCoil. */

    //=========================================================================================================
    /**
     * Constructs the Forward Coil
     */
    FwdCoil(int p_np);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FwdCoil      FwdCoil which should be copied.
     */
    FwdCoil(const FwdCoil& p_FwdCoil);

    //=========================================================================================================
    /**
     * Destroys the Forward Coil description
     */
    ~FwdCoil();

    //=========================================================================================================
    /**
     * Create an electrode definition. Transform coordinate frame if so desired.
     *
     * @param[in] ch     Channel information to use.
     * @param[in] t      Transform the points using this.
     *
     * @return   The created coil.
     */
    static FwdCoil::UPtr create_eeg_el(const FIFFLIB::FiffChInfo& ch,
                                        const FIFFLIB::FiffCoordTrans& t = FIFFLIB::FiffCoordTrans());

    //=========================================================================================================
    /**
     * Checks if this is an axial coil.
     *
     * @return   True if axial coil, false otherwise.
     */
    bool is_axial_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an magnetometer.
     *
     * @return   True if magnetometer, false otherwise.
     */
    bool is_magnetometer_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an planar coil.
     *
     * @return   True if planar coil, false otherwise.
     */
    bool is_planar_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an EEG electrode.
     *
     * @return   True if EEG electrode, false otherwise.
     */
    bool is_eeg_electrode() const;

public:
    QString chname;         /**< Name of this channel. */
    int     coord_frame;    /**< Which coordinate frame are we in?. */
    QString desc;           /**< Description for this type of a coil. */
    int     coil_class;     /**< Coil class. */
    int     type;           /**< Coil type. */
    int     accuracy;       /**< Accuracy. */
    float   size;           /**< Coil size. */
    float   base;           /**< Baseline. */
    Eigen::Vector3f r0;     /**< Coil coordinate system origin. */
    Eigen::Vector3f ex;     /**< Coil coordinate system x unit vector. */
    Eigen::Vector3f ey;     /**< Coil coordinate system y unit vector. */
    Eigen::Vector3f ez;     /**< Coil coordinate system z unit vector. */
    int     np;             /**< Number of integration points. */
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> rmag;    /**< The field point locations (np x 3). */
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> cosmag;  /**< The corresponding direction cosines (np x 3). */
    Eigen::VectorXf w;              /**< The weighting coefficients. */

    /** Return a read-only map to the j-th integration point position (3 contiguous floats). */
    Eigen::Map<const Eigen::Vector3f> pos(int j) const { return Eigen::Map<const Eigen::Vector3f>(rmag.row(j).data()); }
    /** Return a read-only map to the j-th integration point direction cosine. */
    Eigen::Map<const Eigen::Vector3f> dir(int j) const { return Eigen::Map<const Eigen::Vector3f>(cosmag.row(j).data()); }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_COIL_H
