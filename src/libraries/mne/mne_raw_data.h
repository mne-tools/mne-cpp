//=============================================================================================================
/**
 * @file     mne_raw_data.h
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
 * @brief    MneRawData class declaration.
 *
 */

#ifndef MNERAWDATA_H
#define MNERAWDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>
#include "mne_raw_info.h"
#include "mne_raw_buf_def.h"
#include "mne_proj_op.h"
#include "mne_sss_data.h"
#include "mne_ctf_comp_data_set.h"
#include "mne_deriv.h"
#include "mne_deriv_set.h"
#include "mne_types.h"

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

//=============================================================================================================
/**
 * Implements the MNE Raw Data (Replaces *mneRawData,mneRawDataRec; struct of MNE-C mne_types.h).
 *
 * @brief A comprehensive raw data structure
 */
class MNESHARED_EXPORT MneRawData
{
public:
    typedef QSharedPointer<MneRawData> SPtr;              /**< Shared pointer type for MneRawData. */
    typedef QSharedPointer<const MneRawData> ConstSPtr;   /**< Const shared pointer type for MneRawData. */

    //=========================================================================================================
    /**
     * Constructs the MNE Raw Data
     * Refactored: new_raw_data (mne_raw_data.c)
     */
    MneRawData();

    //=========================================================================================================
    /**
     * Destroys the MNE Raw Data
     * Refactored: mne_raw_free_data (mne_raw_data.c)
     */
    ~MneRawData();

    /**
     * Create a frequency-domain filter response from the current filter
     * definition and sampling frequency, replacing any existing response.
     *
     * @param[out] highpass_effective  Set to non-zero if the highpass filter is active.
     */
    void add_filter_response(int *highpass_effective);

    /**
     * Allocate filter output buffers spanning the entire raw data range
     * (with taper overlap), initialize a ring buffer for filtered data,
     * and create the filter response.
     */
    void setup_filter_bufs();

    /**
     * Read a single raw data buffer from the FIFF file into memory,
     * allocating from the ring buffer if needed.
     *
     * @param[in, out] buf   The buffer definition to load.
     *
     * @return OK on success, FAIL on read error.
     */
    int load_one_buffer(MneRawBufDef* buf);

    /**
     * Apply CTF compensation to a loaded raw data buffer if its
     * compensation status differs from the desired one.
     *
     * @param[in, out] buf   The buffer to compensate.
     *
     * @return OK on success, FAIL on error.
     */
    int compensate_buffer(MneRawBufDef* buf);

    /**
     * Extract raw data samples for selected channels (including derived
     * channels) with compensation applied, zero-padding or edge-extending
     * as needed at boundaries.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[in]  ns       Number of samples to extract.
     * @param[out] picked   Array of per-channel data vectors (pre-allocated).
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data(mneChSelection sel,
                  int            firsts,
                  int            ns,
                  float          **picked);

    /**
     * Extract raw data with SSP projection applied to each time sample
     * before channel selection.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[in]  ns       Number of samples to extract.
     * @param[out] picked   Array of per-channel data vectors (pre-allocated).
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data_proj(mneChSelection sel,
                       int            firsts,
                       int            ns,
                       float          **picked);

    /**
     * Load a single filter-sized buffer by picking projected unfiltered data
     * with taper margins for overlap-add filtering.
     *
     * @param[in, out] buf   The filter buffer definition to load.
     *
     * @return OK on success, FAIL on error.
     */
    int load_one_filt_buf(MneRawBufDef* buf);

    /**
     * Pick filtered data for a channel selection by loading filter buffers,
     * applying the frequency-domain filter per channel (skipping stimulus
     * channels), and accumulating overlapping results.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[in]  ns       Number of samples to extract.
     * @param[out] picked   Array of per-channel data vectors (pre-allocated).
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data_filt(mneChSelection sel,
                       int            firsts,
                       int            ns,
                       float          **picked);

    /**
     * Open a raw FIFF file with explicit compensation grade.
     *
     * Reads channel info, compensation data, SSS data, sets up data/filter
     * buffers and bad channels, and returns a fully initialized raw data object.
     *
     * @param[in] name              Path to the raw FIFF file.
     * @param[in] omit_skip         If non-zero, discard initial skip samples.
     * @param[in] allow_maxshield   If non-zero, allow MaxShield data.
     * @param[in] filter            Filter definition to apply.
     * @param[in] comp_set          Desired compensation grade (-1 for file native).
     *
     * @return A new raw data object, or NULL on failure. Caller takes ownership.
     */
    static MneRawData* open_file_comp(const QString& name, int omit_skip, int allow_maxshield, mneFilterDef filter, int comp_set);

    /**
     * Open a raw FIFF file using the file's native compensation grade.
     * Convenience wrapper for open_file_comp() with comp_set = -1.
     *
     * @param[in] name              Path to the raw FIFF file.
     * @param[in] omit_skip         If non-zero, discard initial skip samples.
     * @param[in] allow_maxshield   If non-zero, allow MaxShield data.
     * @param[in] filter            Filter definition to apply.
     *
     * @return A new raw data object, or NULL on failure. Caller takes ownership.
     */
    static MneRawData* open_file(const QString& name, int omit_skip, int allow_maxshield, mneFilterDef filter);

public:
    QString         filename;             /**< Path to the raw FIFF file. */
    //  FIFFLIB::fiffFile       file;
    FIFFLIB::FiffStream::SPtr stream;     /**< Open FIFF stream for reading. */
    std::unique_ptr<MNELIB::MneRawInfo> info;      /**< Raw data information loaded using MNE routines. */
    QStringList     ch_names;           /**< Channel names as a flat list. */
    QStringList     badlist;            /**< Bad channel names. */
    int             nbad;               /**< Number of bad channels. */
    int              *bad;              /**< Boolean array marking bad channels. */
    MNELIB::MneRawBufDef* bufs;         /**< Raw data buffer definitions. */
    int              nbuf;                  /**< Number of raw data buffers. */
    MNELIB::MneRawBufDef* filt_bufs;    /**< Filtered data buffer definitions. */
    int             nfilt_buf;              /**< Number of filtered buffers. */
    int             first_samp;         /**< First sample index in the file. */
    int             omit_samp;          /**< Number of skip samples omitted from the beginning. */
    int             first_samp_old;     /**< First sample index (old-version compatible value). */
    int             omit_samp_old;      /**< Omitted samples (old-version compatible value). */
    int             nsamp;              /**< Total number of samples in the file. */
    float           *first_sample_val;  /**< Values at the first sample (for DC offset correction before filtering). */
    std::unique_ptr<MNELIB::MneProjOp> proj;            /**< SSP projection operator. */
    std::unique_ptr<MNELIB::MneSssData> sss;            /**< SSS data found in this file. */
    std::unique_ptr<MNELIB::MneCTFCompDataSet> comp;    /**< CTF compensation data. */
    int             comp_file;          /**< Compensation grade stored in the file. */
    int             comp_now;           /**< Current compensation grade applied to data. */
    mneFilterDef    filter;             /**< Filter definition (highpass/lowpass). */
    void            *filter_data;       /**< Opaque filter state data. */
    mneUserFreeFunc filter_data_free;   /**< Function to free filter_data. */
    mneEventList    event_list;         /**< Trigger event list. */
    unsigned int    max_event;          /**< Maximum event number in use. */
    QString         dig_trigger;        /**< Name of the digital trigger channel. */
    unsigned int     dig_trigger_mask;  /**< Bit mask applied to digital trigger channel. */
    float            *offsets;          /**< DC offset corrections for display. */
    void             *ring;             /**< Ring buffer for raw data (opaque). */
    void             *filt_ring;        /**< Ring buffer for filtered data (opaque). */
    std::unique_ptr<MNELIB::MneDerivSet> deriv;        /**< Derivation data definitions. */
    std::unique_ptr<MNELIB::MneDeriv> deriv_matched;   /**< Derivation data matched to this raw data. */
    float            *deriv_offsets;        /**< DC offset corrections for derived channels. */
    void             *user;         /**< User-defined data pointer. */
    mneUserFreeFunc  user_free;     /**< Function to free user data. */

//// ### OLD STRUCT ###
//typedef struct {        /* A comprehensive raw data structure */
//    char             *filename;             /* This is our file */
//    //  FIFFLIB::fiffFile       file;
//    FIFFLIB::FiffStream::SPtr stream;
//    MNELIB::MneRawInfo*      info;      /* Loaded using the mne routines */
//    char             **ch_names;            /* Useful to have the channel names as a single list */
//    char             **badlist;             /* Bad channel names */
//    int              nbad;                  /* How many? */
//    int              *bad;                  /* Which channels are bad? */
//    MNELIB::MneRawBufDef* bufs;         /* These are the data */
//    int              nbuf;                  /* How many? */
//    MNELIB::MneRawBufDef* filt_bufs;    /* These are the filtered ones */
//    int              nfilt_buf;
//    int              first_samp;            /* First sample? */
//    int              omit_samp;             /* How many samples of skip omitted in the beginning */
//    int              first_samp_old;        /* This is the value first_samp would have in the old versions */
//    int              omit_samp_old;         /* This is the value omit_samp would have in the old versions */
//    int              nsamp;                 /* How many samples in total? */
//    float            *first_sample_val;     /* Values at the first sample (for dc offset correction before filtering) */
//    MNELIB::MneProjOp* proj;            /* Projection operator */
//    MNELIB::MneSssData* sss;            /* SSS data found in this file */
//    MNELIB::MneCTFCompDataSet* comp;    /* Compensation data */
//    int              comp_file;             /* Compensation status of these raw data in file */
//    int              comp_now;              /* Compensation status of these raw data in file */
//    mneFilterDef     filter;                /* Filter definition */
//    void             *filter_data;          /* This can be whatever the filter needs */
//    mneUserFreeFunc  filter_data_free;      /* Function to free the above */
//    mneEventList     event_list;            /* Trigger events */
//    unsigned int     max_event;             /* Maximum event number in usenest */
//    char             *dig_trigger;          /* Name of the digital trigger channel */
//    unsigned int     dig_trigger_mask;      /* Mask applied to digital trigger channel before considering it */
//    float            *offsets;              /* Dc offset corrections for display */
//    void             *ring;                 /* The ringbuffer (structure is of no interest to us) */
//    void             *filt_ring;            /* Separate ring buffer for filtered data */
//    MNELIB::MneDerivSet*  deriv;        /* Derivation data */
//    MNELIB::MneDeriv*     deriv_matched;/* Derivation data matched to this raw data and collected into a single item */
//    float            *deriv_offsets;        /* Dc offset corrections for display of the derived channels */
//    void             *user;	        /* Whatever */
//    mneUserFreeFunc  user_free;     /* How this is freed */
//} *mneRawData,mneRawDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNERAWDATA_H
