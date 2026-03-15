//=============================================================================================================
/**
 * @file     fwd_coil.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FwdCoil class declaration.
 *
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
 * Implements FwdCoil (Replaces *fwdCoil,fwdCoilRec; struct of MNE-C fwd_types.h).
 *
 * @brief Single MEG or EEG sensor coil with integration points, weights, and coordinate frame.
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
