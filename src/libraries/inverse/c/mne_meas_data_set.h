//=============================================================================================================
/**
 * @file     mne_meas_data_set.h
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
 * @brief    MNE Meas Data Set (MneMeasDataSet) class declaration.
 *
 */

#ifndef MNEMEASDATASET_H
#define MNEMEASDATASET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

typedef void (*mneUserFreeFuncTmp_1)(void *);  /* General purpose */

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneMneData;

//=============================================================================================================
/**
 * Implements MNE Meas Data Set (Replaces *mneMeasDataSet,mneMeasDataSetRec; struct of MNE-C mne_types.h).
 *
 * @brief One data set, used in mneMeasData
 */
class INVERSESHARED_EXPORT MneMeasDataSet
{
public:
    typedef QSharedPointer<MneMeasDataSet> SPtr;              /**< Shared pointer type for MneMeasDataSet. */
    typedef QSharedPointer<const MneMeasDataSet> ConstSPtr;   /**< Const shared pointer type for MneMeasDataSet. */

    //=========================================================================================================
    /**
     * Constructs the MNE Meas Data Set
     * Refactored: mne_new_meas_data_set (mne_read_data.c)
     */
    MneMeasDataSet();

    //=========================================================================================================
    /**
     * Destroys the MNE Meas Data Set description
     * Refactored: mne_free_meas_data_set (mne_read_data.c)
     */
    ~MneMeasDataSet();

public:
    /*
     * These are unique to each data set
     */
    QString         comment;       /* Comment associated with these data */
    float           **data;         /* The measured data */
    float           **data_proj;    /* Some programs maybe interested in keeping the data after SSP separately */
    float           **data_filt;    /* Some programs maybe interested in putting a filtered version here */
    float           **data_white;   /* The whitened data */
    float           *stim14;        /* Data from the digital stimulus channel */
    int             first;          /* First sample index for raw data processing */
    int             np;             /* Number of times */
    int             nave;           /* Number of averaged responses */
    int             kind;           /* Which aspect of data */
    float           tmin;           /* Starting time */
    float           tstep;          /* Time step */
    float           *baselines;     /* Baseline values currently applied to the data */
    MneMneData*     mne;            /* These are the data associated with MNE computations */
    void            *user_data;     /* Anything else we want */
    mneUserFreeFuncTmp_1 user_data_free; /* Function to set the above free */

// ### OLD STRUCT ###
//typedef struct {		 /* One data set, used in mneMeasData */
//    /*
//    * These are unique to each data set
//    */
//    char            *comment;       /* Comment associated with these data */
//    float           **data;         /* The measured data */
//    float           **data_proj;    /* Some programs maybe interested in keeping the data after SSP separately */
//    float           **data_filt;    /* Some programs maybe interested in putting a filtered version here */
//    float           **data_white;   /* The whitened data */
//    float           *stim14;        /* Data from the digital stimulus channel */
//    int             first;          /* First sample index for raw data processing */
//    int             np;             /* Number of times */
//    int             nave;           /* Number of averaged responses */
//    int             kind;           /* Which aspect of data */
//    float           tmin;           /* Starting time */
//    float           tstep;          /* Time step */
//    float           *baselines;     /* Baseline values currently applied to the data */
//    INVERSELIB::MneMneData*   mne;  /* These are the data associated with MNE computations */
//    void            *user_data;     /* Anything else we want */
//    mneUserFreeFunc user_data_free; /* Function to set the above free */
//} *mneMeasDataSet,mneMeasDataSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVERSELIB

#endif // MNEMEASDATASET_H
