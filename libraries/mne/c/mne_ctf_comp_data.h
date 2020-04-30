//=============================================================================================================
/**
 * @file     mne_ctf_comp_data.h
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
 * @brief    MneCTFCompData class declaration.
 *
 */

#ifndef MNECTFCOMPDATA_H
#define MNECTFCOMPDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

#include "mne_named_matrix.h"
#include <fiff/c/fiff_sparse_matrix.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements an MNE CTF Compensation Data (Replaces *mneCTFcompData,mneCTFcompDataRec; struct of MNE-C mne_types.h).
 *
 * @brief One MNE CTF compensation description
 */
class MNESHARED_EXPORT MneCTFCompData
{
public:
    typedef QSharedPointer<MneCTFCompData> SPtr;              /**< Shared pointer type for MneCTFCompData. */
    typedef QSharedPointer<const MneCTFCompData> ConstSPtr;   /**< Const shared pointer type for MneCTFCompData. */

    //=========================================================================================================
    /**
     * Constructs the MNE CTF Comepnsation Data
     * Refactored: mne_new_ctf_comp_data (mne_ctf_comp.c)
     */
    MneCTFCompData();

    //=========================================================================================================
    /**
     * Copies MNE CTF Comepnsation Data
     * Refactored: mne_dup_ctf_comp_data (mne_ctf_comp.c)
     */
    MneCTFCompData(const MneCTFCompData& comp);

    //=========================================================================================================
    /**
     * Destroys the MNE CTF Comepnsation Data
     * Refactored: mne_free_ctf_comp_data (mne_ctf_comp.c)
     */
    ~MneCTFCompData();

    static int mne_calibrate_ctf_comp(MneCTFCompData*       one,
                                      const QList<FIFFLIB::FiffChInfo> &chs,
                                      int            nch,
                                      int            do_it);

public:
    int             kind;                   /* The compensation kind (CTF) */
    int             mne_kind;               /* Our kind */
    int             calibrated;             /* Are the coefficients in the file calibrated already? */
    MneNamedMatrix*  data;      /* The compensation data */
    FIFFLIB::FiffSparseMatrix* presel;   /* Apply this selector prior to compensation */
    FIFFLIB::FiffSparseMatrix* postsel;  /* Apply this selector after compensation */
    float           *presel_data;           /* These are used for the intermediate results in the calculations */
    float           *comp_data;
    float           *postsel_data;

//// ### OLD STRUCT ###
//typedef struct {
//    int             kind;                   /* The compensation kind (CTF) */
//    int             mne_kind;               /* Our kind */
//    int             calibrated;             /* Are the coefficients in the file calibrated already? */
//    MNELIB::MneNamedMatrix*  data;      /* The compensation data */
//    MNELIB::FiffSparseMatrix* presel;   /* Apply this selector prior to compensation */
//    MNELIB::FiffSparseMatrix* postsel;  /* Apply this selector after compensation */
//    float           *presel_data;           /* These are used for the intermediate results in the calculations */
//    float           *comp_data;
//    float           *postsel_data;
//} *mneCTFcompData,mneCTFcompDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECTFCOMPDATA_H
