//=============================================================================================================
/**
 * @file     mne_inverse_operator.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Inverse Operator (MneInverseOperator) class declaration.
 *
 */

#ifndef MNEINVERSEOPERATOR_H
#define MNEINVERSEOPERATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include <fiff/fiff_types.h>
#include <mne/c/mne_surface_or_volume.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{
    class FiffCoordTransOld;
}

namespace MNELIB
{
    class MneCovMatrix;
    class MneNamedMatrix;
    class MneProjOp;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements an MNE Inverse Operator (Replaces *mneInverseOperator,mneInverseOperatorRec; struct of MNE-C mne_types.h).
 *
 * @brief An inverse operator
 */
class INVERSESHARED_EXPORT MneInverseOperator
{
public:
    typedef QSharedPointer<MneInverseOperator> SPtr;              /**< Shared pointer type for MneInverseOperator. */
    typedef QSharedPointer<const MneInverseOperator> ConstSPtr;   /**< Const shared pointer type for MneInverseOperator. */

    //=========================================================================================================
    /**
     * Constructs the MNE Inverse Operator
     * Refactored:  (.c)
     */
    MneInverseOperator();

    //=========================================================================================================
    /**
     * Destroys the MNE Inverse Operator
     * Refactored:  (.c)
     */
    ~MneInverseOperator();

public:
    FIFFLIB::fiffId meas_id;            /* The assosiated measurement ID */
    MNELIB::MneSourceSpaceOld* *spaces; /* The source spaces */
    int            nspace;              /* Number of source spaces */
    FIFFLIB::FiffCoordTransOld* meg_head_t;  /* MEG device <-> head coordinate transformation */
    FIFFLIB::FiffCoordTransOld* mri_head_t;  /* MRI device <-> head coordinate transformation */
    int            methods;         /* EEG, MEG or EEG+MEG (see mne_fiff.h) */
    int            nchan;           /* Number of measurement channels */
    int            nsource;         /* Number of source points */
    int            fixed_ori;       /* Fixed source orientations? */
    float          **rr_source;     /* The active source points */
    float          **nn_source;     /* The source orientations (These are equal to the cortex normals in the fixed orientation case) */
    int            coord_frame;             /* Which coordinates are the locations and orientations given in? */
    MNELIB::MneCovMatrix*  sensor_cov; /* Sensor covariance matrix */
    int            nave;                    /* Number of averaged responses (affects scaling of the noise covariance) */
    int            current_unit;            /* This can be FIFF_UNIT_AM, FIFF_UNIT_AM_M2, FIFF_UNIT_AM_M3 */
    MNELIB::MneCovMatrix*   source_cov;     /* Source covariance matrix */
    MNELIB::MneCovMatrix*   orient_prior;   /* Orientation prior applied */
    MNELIB::MneCovMatrix*   depth_prior;    /* Depth-weighting prior applied */
    MNELIB::MneCovMatrix*   fMRI_prior;     /* fMRI prior applied */
    float          *sing;                       /* Singular values of the inverse operator */
    MNELIB::MneNamedMatrix* eigen_leads;    /* The eigen leadfields */
    int            eigen_leads_weighted;        /* Have the above been already weighted with R^0.5? */
    MNELIB::MneNamedMatrix* eigen_fields;   /* Associated field patterns */
    float          trace_ratio;                 /* tr(GRG^T)/tr(C) */
    MNELIB::MneProjOp*    proj;             /* The associated projection operator */

// ### OLD STRUCT ###
//typedef struct {                    /* An inverse operator */
//    FIFFLIB::fiffId         meas_id;                            /* The assosiated measurement ID */
//    INVERSELIB::MneSourceSpaceOld* *spaces;   /* The source spaces */
//    int            nspace;                      /* Number of source spaces */
//    INVERSELIB::FiffCoordTransOld* meg_head_t;  /* MEG device <-> head coordinate transformation */
//    INVERSELIB::FiffCoordTransOld* mri_head_t;  /* MRI device <-> head coordinate transformation */
//    int            methods;         /* EEG, MEG or EEG+MEG (see mne_fiff.h) */
//    int            nchan;           /* Number of measurement channels */
//    int            nsource;         /* Number of source points */
//    int            fixed_ori;       /* Fixed source orientations? */
//    float          **rr_source;     /* The active source points */
//    float          **nn_source;     /* The source orientations (These are equal to the cortex normals in the fixed orientation case) */
//    int            coord_frame;             /* Which coordinates are the locations and orientations given in? */
//    INVERSELIB::MneCovMatrix*   sensor_cov; /* Sensor covariance matrix */
//    int            nave;                    /* Number of averaged responses (affects scaling of the noise covariance) */
//    int            current_unit;            /* This can be FIFF_UNIT_AM, FIFF_UNIT_AM_M2, FIFF_UNIT_AM_M3 */
//    INVERSELIB::MneCovMatrix*   source_cov;     /* Source covariance matrix */
//    INVERSELIB::MneCovMatrix*   orient_prior;   /* Orientation prior applied */
//    INVERSELIB::MneCovMatrix*   depth_prior;    /* Depth-weighting prior applied */
//    INVERSELIB::MneCovMatrix*   fMRI_prior;     /* fMRI prior applied */
//    float          *sing;                       /* Singular values of the inverse operator */
//    INVERSELIB::MneNamedMatrix* eigen_leads;    /* The eigen leadfields */
//    int            eigen_leads_weighted;        /* Have the above been already weighted with R^0.5? */
//    INVERSELIB::MneNamedMatrix* eigen_fields;   /* Associated field patterns */
//    float          trace_ratio;                 /* tr(GRG^T)/tr(C) */
//    INVERSELIB::MneProjOp*    proj;             /* The associated projection operator */
//} *mneInverseOperator,mneInverseOperatorRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVERSELIB

#endif // MNEINVERSEOPERATOR_H
