//=============================================================================================================
/**
 * @file     mne_meas_data.h
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
 * @brief    MNE Meas Data (MneMeasData) class declaration.
 *
 */

#ifndef MNEMEASDATA_H
#define MNEMEASDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include <fiff/fiff_types.h>

#include <mne/c/mne_types.h>
#include <mne/c/mne_raw_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MneNamedMatrix;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

class MneInverseOperator;
class MneMeasDataSet;

//=============================================================================================================
/**
 * Implements MNE Meas Data (Replaces *mneMeasData,mneMeasDataRec; struct of MNE-C mne_types.h).
 *
 * @brief easurement data representation in MNE calculations
 */
class INVERSESHARED_EXPORT MneMeasData
{
public:
    typedef QSharedPointer<MneMeasData> SPtr;              /**< Shared pointer type for MneMeasData. */
    typedef QSharedPointer<const MneMeasData> ConstSPtr;   /**< Const shared pointer type for MneMeasData. */

    //=========================================================================================================
    /**
     * Constructs the MNE Meas Data
     * Refactored: mne_new_meas_data (mne_read_data.c)
     */
    MneMeasData();

    //=========================================================================================================
    /**
     * Destroys the MNE Meas Data description
     * Refactored: mne_free_meas_data (mne_read_data.c)
     */
    ~MneMeasData();

    //=========================================================================================================
    /**
     * Change the baseline setting in the current data set
     * Refactored: mne_adjust_baselines (mne_apply_baselines.c)
     *
     * @param[in] bmin   Baseline start timepoint.
     * @param[in] bmax   Baseline end timepoint.
     */
    void adjust_baselines(float bmin, float bmax);

    //============================= mne_read_data.c =============================

    static MneMeasData* mne_read_meas_data_add(const QString&       name,       /* Name of the measurement file */
                                       int                  set,        /* Which data set */
                                       MneInverseOperator*   op,         /* For consistency checks */
                                       MNELIB::MneNamedMatrix*       fwd,        /* Another option for consistency checks */
                                       const QStringList&   namesp,   /* Yet another option: explicit name list */
                                       int                  nnamesp,
                                       MneMeasData*          add_to);

    static MneMeasData* mne_read_meas_data(const QString&       name,       /* Name of the measurement file */
                                   int                  set,        /* Which data set */
                                   MneInverseOperator*  op,         /* For consistency checks */
                                   MNELIB::MneNamedMatrix*      fwd,        /* Another option for consistency checks */
                                   const QStringList&   namesp,   /* Yet another option: explicit name list */
                                   int                  nnamesp);

public:
    QString                 filename;  /* The source file name */
    FIFFLIB::fiffId         meas_id;    /* The id from the measurement file */
    FIFFLIB::fiffTimeRec    meas_date;  /* The measurement date from the file */
    QList<FIFFLIB::FiffChInfo>     chs;        /* The channel information */
    FIFFLIB::FiffCoordTransOld* meg_head_t; /* MEG device <-> head coordinate transformation */
    FIFFLIB::FiffCoordTransOld* mri_head_t; /* MRI device <-> head coordinate transformation (duplicated from the inverse operator or loaded separately) */
    float                   sfreq;      /* Sampling frequency */
    int                     nchan;      /* Number of channels */
    float                   highpass;   /* Highpass filter setting */
    float                   lowpass;    /* Lowpass filter setting */
    MNELIB::MneProjOp*  proj;       /* Associated projection operator (useful if inverse operator is not included) */
    MNELIB::MneCTFCompDataSet*      comp;       /* The software gradient compensation data */
    MneInverseOperator*     op;         /* Associated inverse operator */
    MNELIB::MneNamedMatrix*         fwd;        /* Forward operator for dipole fitting */
    MNELIB::MneRawData*             raw;        /* This will be non-null if the data stems from a raw data file */
    MNELIB::mneChSelection          chsel;      /* Channel selection for raw data */
    QStringList             badlist;  /* Bad channel names */
    int                     nbad;       /* How many? */
    int                     *bad;       /* Which channels are bad? */
    /*
     * These are the data sets loaded
     */
    int                     ch_major;   /* Rows are channels rather than times */
    QList<MneMeasDataSet*>  sets;       /* All loaded data sets */
    int                     nset;       /* How many */
    MneMeasDataSet*         current;    /* Which is the current one */

// ### OLD STRUCT ###
//typedef struct {    /* Measurement data representation in MNE calculations */
///*
//* These are common to all data sets
//*/
//    char                    *filename;  /* The source file name */
//    FIFFLIB::fiffId         meas_id;    /* The id from the measurement file */
//    FIFFLIB::fiffTimeRec    meas_date;  /* The measurement date from the file */
//    FIFFLIB::fiffChInfo     chs;        /* The channel information */
//    INVERSELIB::FiffCoordTransOld* meg_head_t; /* MEG device <-> head coordinate transformation */
//    INVERSELIB::FiffCoordTransOld* mri_head_t; /* MRI device <-> head coordinate transformation (duplicated from the inverse operator or loaded separately) */
//    float               sfreq;      /* Sampling frequency */
//    int                 nchan;      /* Number of channels */
//    float               highpass;   /* Highpass filter setting */
//    float               lowpass;    /* Lowpass filter setting */
//    mneProjOp           proj;       /* Associated projection operator (useful if inverse operator is not included) */
//    mneCTFcompDataSet   comp;       /* The software gradient compensation data */
//    mneInverseOperator  op;         /* Associated inverse operator */
//    mneNamedMatrix      fwd;        /* Forward operator for dipole fitting */
//    mneRawData          raw;        /* This will be non-null if the data stems from a raw data file */
//    mneChSelection      chsel;      /* Channel selection for raw data */
//    char                **badlist;  /* Bad channel names */
//    int                 nbad;       /* How many? */
//    int                 *bad;       /* Which channels are bad? */
//    /*
//    * These are the data sets loaded
//    */
//    int                ch_major;            /* Rows are channels rather than times */
//    INVERSELIB::MneMeasDataSet* *sets;      /* All loaded data sets */
//    int                nset;                /* How many */
//    INVERSELIB::MneMeasDataSet* current;    /* Which is the current one */
//} *mneMeasData,mneMeasDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVERSELIB

#endif // MNEMEASDATA_H
