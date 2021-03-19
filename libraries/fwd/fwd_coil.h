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

#ifndef FWDCOIL_H
#define FWDCOIL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

#include <fiff/c/fiff_coord_trans_old.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#define FWD_COIL_UNKNOWN      0

#define FWD_COILC_UNKNOWN     0
#define FWD_COILC_EEG         1000
#define FWD_COILC_MAG         1
#define FWD_COILC_AXIAL_GRAD  2
#define FWD_COILC_PLANAR_GRAD 3
#define FWD_COILC_AXIAL_GRAD2 4

#define FWD_COIL_ACCURACY_POINT    0
#define FWD_COIL_ACCURACY_NORMAL   1
#define FWD_COIL_ACCURACY_ACCURATE 2

#define FWD_IS_MEG_COIL(x) ((x) != FWD_COILC_EEG && (x) != FWD_COILC_UNKNOWN)

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements FwdCoil (Replaces *fwdCoil,fwdCoilRec; struct of MNE-C fwd_types.h).
 *
 * @brief FwdCoil description
 */
class FWDSHARED_EXPORT FwdCoil
{
public:
    typedef QSharedPointer<FwdCoil> SPtr;              /**< Shared pointer type for FwdCoil. */
    typedef QSharedPointer<const FwdCoil> ConstSPtr;   /**< Const shared pointer type for FwdCoil. */

    //=========================================================================================================
    /**
     * Constructs the Forward Coil
     * Refactored: fwd_new_coil (fwd_coil_def.c)
     */
    FwdCoil(int p_np);

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: fwd_dup_coil (fwd_coil_def.c)
     *
     * @param[in] p_FwdCoil      FwdCoil which should be copied.
     */
    FwdCoil(const FwdCoil& p_FwdCoil);

    //=========================================================================================================
    /**
     * Destroys the Forward Coil description
     * Refactored: fwd_free_coil
     */
    ~FwdCoil();

    //=========================================================================================================
    /**
     * Create an electrode definition. Transform coordinate frame if so desired.
     * Refactored: fwd_create_eeg_el (fwd_coil_def.c)
     *
     * @param[in] ch     Channel information to use.
     * @param[in] t      Transform the points using this.
     *
     * @return   The created coil.
     */
    static FwdCoil* create_eeg_el(const FIFFLIB::FiffChInfo& ch,
                                  const FIFFLIB::FiffCoordTransOld* t);

    //=========================================================================================================
    /**
     * Checks if this is an axial coil.
     * Refactored: fwd_is_axial_coil (fwd_coil_def.c)
     *
     * @return   True if axial coil, false otherwise.
     */
    bool is_axial_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an magnetometer.
     * Refactored: fwd_is_magnetometer_coil (fwd_coil_def.c)
     *
     * @return   True if magnetometer, false otherwise.
     */
    bool is_magnetometer_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an planar coil.
     * Refactored: fwd_is_planar_coil (fwd_coil_def.c)
     *
     * @return   True if planar coil, false otherwise.
     */
    bool is_planar_coil() const;

    //=========================================================================================================
    /**
     * Checks if this is an EEG electrode.
     * Refactored: fwd_is_eeg_electrode (fwd_coil_def.c)
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
    float   r0[3];          /**< Coil coordinate system origin. */
    float   ex[3];          /**< Coil coordinate system unit vectors. */
    float   ey[3];          /**< This stupid construction needs to be replaced with. */
    float   ez[3];          /**< a coordinate transformation. */
    int     np;             /**< Number of integration points. */
    float   **rmag;         /**< The field point locations. */
    float   **cosmag;       /**< The corresponding direction cosines. */
    float   *w;             /**< The weighting coefficients. */

// ### OLD STRUCT ###
//    typedef struct {
//      char         *chname;		/* Name of this channel */
//      int          coord_frame;	/* Which coordinate frame are we in? */
//      char         *desc;	        /* Description for this type of a coil */
//      int          coil_class;	/* Coil class */
//      int          type;		/* Coil type */
//      int          accuracy;	/* Accuracy */
//      float        size;		/* Coil size */
//      float        base;		/* Baseline */
//      float        r0[3];		/* Coil coordinate system origin */
//      float        ex[3];		/* Coil coordinate system unit vectors */
//      float        ey[3];		/* This stupid construction needs to be replaced with */
//      float        ez[3];		/* a coordinate transformation */
//      int          np;		/* Number of integration points */
//      float        **rmag;		/* The field point locations */
//      float        **cosmag;	/* The corresponding direction cosines */
//      float        *w;		/* The weighting coefficients */
//    } *fwdCoil,fwdCoilRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWDCOIL_H
