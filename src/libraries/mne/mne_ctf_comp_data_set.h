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

#include "mne_global.h"

#include "mne_named_matrix.h"
#include <fiff/fiff_sparse_matrix.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

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
 * @brief Collection of CTF third-order gradient compensation operators and the compiled
 *        current/undo pair used to switch between compensation grades at runtime.
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

    /**
     * Read all CTF compensation data sets (matrices and channel info) from
     * a FIFF file, calibrate them, and return a populated set.
     *
     * @param[in] name   Path to the FIFF file.
     *
     * @return A new compensation data set, or NULL on failure. Caller takes ownership.
     */
    static MneCTFCompDataSet* read(const QString& name);

    /**
     * Build the current compensation operator for the given channel set
     * by locating the matching compensation matrix and constructing
     * pre/post-selection sparse matrices.
     *
     * @param[in] chs        Channels to compensate (may include non-MEG channels).
     * @param[in] nch        Number of channels.
     * @param[in] compchs    Compensation input channels (may include non-reference channels).
     * @param[in] ncomp      Number of compensation input channels.
     *
     * @return OK on success, FAIL on error.
     */
    int make_comp(const QList<FIFFLIB::FiffChInfo>& chs,        /* Channels to compensate These may contain channels other than those requiring compensation */
                  int nch,        /* How many of these */
                  QList<FIFFLIB::FiffChInfo> compchs,    /* The compensation input channels These may contain channels other than the MEG compensation channels */
                  int ncomp);

    /**
     * Write the compensation grade into the upper 16 bits of coil_type
     * for all MEG channels in the list.
     *
     * @param[in, out] chs    Channel information list.
     * @param[in]      nch    Number of channels.
     * @param[in]      comp   Compensation grade to set.
     *
     * @return Number of channels modified.
     */
    static int set_comp(QList<FIFFLIB::FiffChInfo> &chs,
                        int        nch,
                        int        comp);

    /**
     * Apply or revert CTF compensation on a single-sample data vector
     * using the current compensation operator.
     *
     * @param[in]      do_it      If TRUE, subtract compensated component; if FALSE, add it back.
     * @param[in, out] data       Data vector to process (ndata elements).
     * @param[in]      ndata      Length of the data vector.
     * @param[in]      compdata   Compensation channel data (ncompdata elements).
     * @param[in]      ncompdata  Length of the compensation data vector.
     *
     * @return OK on success, FAIL on error.
     */
    int apply(int                  do_it,
              float                *data,      /* The data to process */
              int                  ndata,
              float                *compdata,  /* Data containing the compensation channels */
              int                  ncompdata);

    /**
     * Apply or revert CTF compensation across multiple time samples
     * (channels x samples matrix), the transposed equivalent of apply().
     *
     * @param[in]      do_it   If TRUE, apply compensation; if FALSE, revert.
     * @param[in, out] data    Channel-by-channel data array (ndata channels x ns samples).
     * @param[in]      ndata   Number of channels.
     * @param[in]      ns      Number of time samples.
     *
     * @return OK on success, FAIL on error.
     */
    int apply_transpose(int               do_it,
                        float             **data,  /* The data to process (channel by channel) */
                        int               ndata,
                        int               ns);

    /**
     * Extract the compensation grade from MEG channel coil types.
     *
     * @param[in] chs   Channel information list.
     * @param[in] nch   Number of channels.
     *
     * @return The uniform compensation grade, or FAIL if channels have
     *         inconsistent compensation.
     */
    static int get_comp(const QList<FIFFLIB::FiffChInfo>& chs,int nch);

    /**
     * Map a gradient compensation integer code to the corresponding CTF
     * compensation constant.
     *
     * @param[in] grad   Simple integer order (0, 1, 2, 3, ...).
     *
     * @return The mapped CTF compensation constant, or the input if no mapping exists.
     */
    /*
     * Mapping from simple integer orders to the mysterious CTF compensation numbers
     */
    static int map_comp_kind(int grad);

    /**
     * Return a human-readable description string for a compensation kind constant.
     *
     * @param[in] kind   Compensation kind constant.
     *
     * @return A static string describing the compensation (e.g. "third order gradiometer").
     */
    static const char *explain_comp(int kind);

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
    //    if (make_comp(chs,nchan,comp_chs,ncomp_chan) == FAIL)
    //        goto bad;
    //    /*
    //    * Are we there already?
    //    */
    //    if (set->current && set->current->mne_kind == compensate_to) {
    //        printf("The data were already compensated as desired (%s)\n",explain_comp(set->current->kind));
    //        return OK;
    //    }
    //    /*
    //    * Undo any previous compensation
    //    */
    //    for (k = 0; k < np; k++)
    //        if (apply(FALSE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
    //            goto bad;
    //    if (set->current)
    //        printf("The previous compensation (%s) is now undone\n",explain_comp(set->current->kind));
    //    /*
    //    * Set to new gradient compensation
    //    */
    //    if (compensate_to == MNE_CTFV_NOGRAD) {
    //        set_comp(chs,nchan,compensate_to);
    //        printf("No compensation was requested. Original data have been restored.\n");
    //    }
    //    else {
    //        if (set_comp(chs,nchan,compensate_to) > 0) {
    //            if (set->current)
    //                comp_was = set->current->mne_kind;
    //            if (make_comp(chs,nchan,comp_chs,ncomp_chan) == FAIL)
    //                goto bad;
    //            /*
    //            * Do the third-order gradient compensation
    //            */
    //            for (k = 0; k < np; k++)
    //                if (apply(TRUE,data[k],nchan,comp_data[k],ncomp_chan) == FAIL)
    //                    goto bad;
    //            if (set->current)
    //                printf("The data are now compensated as requested (%s).\n",explain_comp(set->current->kind));
    //        }
    //        else
    //            printf("No MEG channels to compensate.\n");
    //    }
    //    return OK;

    //bad : {
    //        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
    //            set_comp(chs,nchan,comp_was);
    //        return FAIL;
    //    }
    //}

    /**
     * Configure the full compensation pipeline: determine current compensation,
     * build undo and target operators, and update channel coil types accordingly.
     *
     * @param[in]      compensate_to  Desired compensation grade.
     * @param[in, out] chs            Channels to compensate (coil_type is updated).
     * @param[in]      nchan          Number of channels.
     * @param[in]      comp_chs       Compensation input channels.
     * @param[in]      ncomp_chan     Number of compensation input channels.
     *
     * @return OK on success, FAIL on error.
     */
    int set_compensation(int compensate_to,  /* What is the desired compensation to achieve */
                         QList<FIFFLIB::FiffChInfo>& chs,            /* The channels to compensate */
                         int nchan,          /* How many? */
                         QList<FIFFLIB::FiffChInfo> comp_chs,       /* Maybe a different set, defaults to the same */
                         int ncomp_chan);

public:
    QList<MneCTFCompData*> comps;   /**< All available compensation data sets. */
    int            ncomp;           /**< Number of compensation data sets. */
    QList<FIFFLIB::FiffChInfo>     chs;    /**< Channel information associated with compensation. */
    int            nch;             /**< Number of channels. */
    std::unique_ptr<MneCTFCompData> undo;           /**< Compensation data to undo the current state. */
    std::unique_ptr<MneCTFCompData> current;        /**< Compiled compensation operator for the current target grade. */

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
