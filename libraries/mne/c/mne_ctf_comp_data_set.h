//=============================================================================================================
/**
 * @file     mne_ctf_comp_data_set.h
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
 * @brief    MneCTFCompDataSet class declaration.
 *
 */

#ifndef MNECTFCOMPDATASET_H
#define MNECTFCOMPDATASET_H

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

class MneCTFCompData;

//=============================================================================================================
/**
 * Implements an MNE CTF Compensation Data Set (Replaces *mneCTFcompDataSet,mneCTFcompDataSetRec; struct of MNE-C mne_types.h).
 *
 * @brief One MNE CTF Compensation Data Set description
 */
class MNESHARED_EXPORT MneCTFCompDataSet
{
public:
    typedef QSharedPointer<MneCTFCompDataSet> SPtr;             /**< Shared pointer type for MneCTFCompDataSet. */
    typedef QSharedPointer<const MneCTFCompDataSet> ConstSPtr;  /**< Const shared pointer type for MneCTFCompDataSet. */

    //=========================================================================================================
    /**
     * Constructs the MNE CTF Comepnsation Data Set
     * Refactored: mne_new_ctf_comp_data_set (mne_ctf_comp.c)
     */
    MneCTFCompDataSet();

    //=========================================================================================================
    /**
     * Copies the MNE CTF Comepnsation Data Set
     * Refactored: mne_dup_ctf_comp_data_set (mne_ctf_comp.c)
     */
    MneCTFCompDataSet(const MneCTFCompDataSet& set);

    //=========================================================================================================
    /**
     * Destroys the MNE CTF Comepnsation Data Set
     * Refactored: mne_free_ctf_comp_data_set (mne_ctf_comp.c)
     */
    ~MneCTFCompDataSet();

    static MneCTFCompDataSet* mne_read_ctf_comp_data(const QString& name);

    static int mne_make_ctf_comp(MneCTFCompDataSet* set,        /* The available compensation data */
                          const QList<FIFFLIB::FiffChInfo>& chs,        /* Channels to compensate These may contain channels other than those requiring compensation */
                          int nch,        /* How many of these */
                          QList<FIFFLIB::FiffChInfo> compchs,    /* The compensation input channels These may contain channels other than the MEG compensation channels */
                          int ncomp);

    static int mne_set_ctf_comp(QList<FIFFLIB::FiffChInfo> &chs,
                         int        nch,
                         int        comp);

    static int mne_apply_ctf_comp(MneCTFCompDataSet*   set,        /* The compensation data */
                           int                  do_it,
                           float                *data,      /* The data to process */
                           int                  ndata,
                           float                *compdata,  /* Data containing the compensation channels */
                           int                  ncompdata);

    static int mne_apply_ctf_comp_t(MneCTFCompDataSet* set,     /* The compensation data */
                             int               do_it,
                             float             **data,  /* The data to process (channel by channel) */
                             int               ndata,
                             int               ns);

    static int mne_get_ctf_comp(const QList<FIFFLIB::FiffChInfo>& chs,int nch);

    /*
     * Mapping from simple integer orders to the mysterious CTF compensation numbers
     */
    static int mne_map_ctf_comp_kind(int grad);

    static const char *mne_explain_ctf_comp(int kind);

    //int mne_ctf_compensate_to(mneCTFcompDataSet set,            /* The compensation data */
    //                          int               compensate_to,  /* What is the desired compensation to achieve */
    //                          fiffChInfo        chs,            /* The channels to compensate */
    //                          int               nchan,          /* How many? */
    //                          fiffChInfo        comp_chs,       /* Maybe a different set, defaults to the same */
    //                          int               ncomp_chan,     /* How many */
    //                          float             **data,         /* The data in a np x nchan matrix allocated with ALLOC_CMATRIX(np,nchan) */
    //                          float             **comp_data,    /* The compensation data in a np x ncomp_chan matrix, defaults to data */
    //                          int               np)             /* How many time points */
    ///*
    //* Make data which has the third-order gradient compensation applied
    //*/
    //{
    //    int k;
    //    int have_comp_chs;
    //    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    //    if (!comp_data)
    //        comp_data = data;
    //    if (!comp_chs) {
    //        comp_chs = chs;
    //        ncomp_chan = nchan;
    //    }
    //    if (set) {
    //        mne_free_ctf_comp_data(set->undo); set->undo = NULL;
    //        mne_free_ctf_comp_data(set->current); set->current = NULL;
    //    }
    //    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
    //        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
    //            have_comp_chs++;
    //    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
    //        printf("No compensation channels in these data.");
    //        return FAIL;
    //    }
    //    /*
    //    * Update the 'current' field in 'set' to reflect the compensation possibly present in the data now
    //    */
    //    if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
    //        goto bad;
    //    /*
    //    * Are we there already?
    //    */
    //    if (set->current && set->current->mne_kind == compensate_to) {
    //        printf("The data were already compensated as desired (%s)\n",mne_explain_ctf_comp(set->current->kind));
    //        return OK;
    //    }
    //    /*
    //    * Undo any previous compensation
    //    */
    //    for (k = 0; k < np; k++)
    //        if (mne_apply_ctf_comp(set,FALSE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
    //            goto bad;
    //    if (set->current)
    //        printf("The previous compensation (%s) is now undone\n",mne_explain_ctf_comp(set->current->kind));
    //    /*
    //    * Set to new gradient compensation
    //    */
    //    if (compensate_to == MNE_CTFV_NOGRAD) {
    //        mne_set_ctf_comp(chs,nchan,compensate_to);
    //        printf("No compensation was requested. Original data have been restored.\n");
    //    }
    //    else {
    //        if (mne_set_ctf_comp(chs,nchan,compensate_to) > 0) {
    //            if (set->current)
    //                comp_was = set->current->mne_kind;
    //            if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
    //                goto bad;
    //            /*
    //            * Do the third-order gradient compensation
    //            */
    //            for (k = 0; k < np; k++)
    //                if (mne_apply_ctf_comp(set,TRUE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
    //                    goto bad;
    //            if (set->current)
    //                printf("The data are now compensated as requested (%s).\n",mne_explain_ctf_comp(set->current->kind));
    //        }
    //        else
    //            printf("No MEG channels to compensate.\n");
    //    }
    //    return OK;

    //bad : {
    //        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
    //            mne_set_ctf_comp(chs,nchan,comp_was);
    //        return FAIL;
    //    }
    //}

    static int mne_ctf_set_compensation(MneCTFCompDataSet* set,            /* The compensation data */
                                 int compensate_to,  /* What is the desired compensation to achieve */
                                 QList<FIFFLIB::FiffChInfo>& chs,            /* The channels to compensate */
                                 int nchan,          /* How many? */
                                 QList<FIFFLIB::FiffChInfo> comp_chs,       /* Maybe a different set, defaults to the same */
                                 int ncomp_chan);

public:
    QList<MneCTFCompData*> comps;   /* All available compensation data sets */
    int            ncomp;           /* How many? */
    QList<FIFFLIB::FiffChInfo>     chs;    /* Channel information */
    int            nch;             /* How many of the above */
    MneCTFCompData* undo;           /* Compensation data to undo the current compensation before applying current */
    MneCTFCompData* current;        /* The current compensation data composed from the above taking into account channels presently available */

//// ### OLD STRUCT ###
//typedef struct {
//    QList<MNELIB::MneCTFCompData*> comps;   /* All available compensation data sets */
//    int            ncomp;                       /* How many? */
//    FIFFLIB::fiffChInfo     chs;                /* Channel information */
//    int            nch;                         /* How many of the above */
//    MNELIB::MneCTFCompData* undo;           /* Compensation data to undo the current compensation before applying current */
//    MNELIB::MneCTFCompData* current;        /* The current compensation data composed from the above taking into account channels presently available */
//} *mneCTFcompDataSet,mneCTFcompDataSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECTFCOMPDATASET_H
