//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_raw_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Legacy MNE-C raw-recording container with per-file buffer descriptors.
 *
 * @ref mne::MNERawData mirrors @c mneRawDataRec: it pairs an
 * @ref mne::FiffInfo with the list of @ref mne::MNERawBufDef entries that
 * describe the on-disk layout of @c FIFF_DATA_BUFFER blocks of a @c
 * -raw.fif file. It is the data source used by the C-ported routines in
 * @c mne_browse_raw / @c mne_make_movie. New code should prefer
 * @ref mne::FiffRawData; this class exists for round-trip parity.
 */

#ifndef MNERAWDATA_H
#define MNERAWDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_raw_info.h"
#include "mne_raw_buf_def.h"
#include "mne_proj_op.h"
#include "mne_sss_data.h"
#include "mne_ctf_comp_data_set.h"
#include "mne_deriv.h"
#include "mne_deriv_set.h"
#include "mne_ch_selection.h"
#include "mne_event_list.h"
#include "mne_filter_def.h"
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

struct RingBuffer;
struct FilterData;

//=============================================================================================================
/**
 * Implements the MNE Raw Data (Replaces *mneRawData,mneRawDataRec; struct of MNE-C mne_types.h).
 *
 * @brief A comprehensive raw data structure
 *
 * @snippet ex_mne_api/main.cpp mne_raw_data_usage
 */
class MNESHARED_EXPORT MNERawData
{
public:
    /** Channels x samples buffer the pick functions fill; each channel's samples are contiguous. */
    using RowMajorMatrixXf = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    //=========================================================================================================
    /**
     * Constructs the MNE Raw Data
     * Refactored: new_raw_data (mne_raw_data.c)
     */
    MNERawData();

    //=========================================================================================================
    /**
     * Destroys the MNE Raw Data
     * Refactored: mne_raw_free_data (mne_raw_data.c)
     */
    ~MNERawData();

    /**
     * Create a frequency-domain filter response from the current filter
     * definition and sampling frequency, replacing any existing response.
     *
     * @param[out] highpass_effective  Set to non-zero if the highpass filter is active.
     */
    void add_filter_response(int* highpass_effective);

    /**
     * Allocate filter output buffers spanning the entire raw data range
     * (with taper overlap), initialize a ring buffer for filtered data,
     * and create the filter response.
     */
    void setup_filter_bufs();

    /**
     * Switch to another filter. Already filtered data is discarded, so later
     * pick_data_filt calls return data filtered with the new definition.
     * Refactored: filter change handling of mne_browse_raw (analyze.c, mne_compare_filters, mne_raw_forget_filtered).
     *
     * @param[in] newFilter  The new filter definition.
     *
     * @return true if the filter changed, false if it equals the current one.
     */
    bool setFilter(const MNEFilterDef& newFilter);

    /**
     * Read a single raw data buffer from the FIFF file into memory,
     * allocating from the ring buffer if needed.
     *
     * @param[in, out] buf   The buffer definition to load.
     *
     * @return OK on success, FAIL on read error.
     */
    int load_one_buffer(MNERawBufDef* buf);

    /**
     * Apply CTF compensation to a loaded raw data buffer if its
     * compensation status differs from the desired one.
     *
     * @param[in, out] buf   The buffer to compensate.
     *
     * @return OK on success, FAIL on error.
     */
    int compensate_buffer(MNERawBufDef* buf);

    /**
     * Extract raw data samples for selected channels (including derived
     * channels) with compensation applied, zero-padding or edge-extending
     * as needed at boundaries.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[out] picked   One row per selected channel (or per channel without a selection) and one
     *                      column per sample; a buffer with the wrong number of rows is refused.
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked);

    /**
     * Extract raw data with SSP projection applied to each time sample
     * before channel selection.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[out] picked   One row per selected channel (or per channel without a selection) and one
     *                      column per sample; a buffer with the wrong number of rows is refused.
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data_proj(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked);

    /**
     * Load a single filter-sized buffer by picking projected unfiltered data
     * with taper margins for overlap-add filtering.
     *
     * @param[in, out] buf   The filter buffer definition to load.
     *
     * @return OK on success, FAIL on error.
     */
    int load_one_filt_buf(MNERawBufDef* buf);

    /**
     * Pick filtered data for a channel selection by loading filter buffers,
     * applying the frequency-domain filter per channel (skipping stimulus
     * channels), and accumulating overlapping results.
     *
     * @param[in]  sel      Channel selection descriptor.
     * @param[in]  firsts   First sample index (absolute).
     * @param[out] picked   One row per selected channel (or per channel without a selection) and one
     *                      column per sample; a buffer with the wrong number of rows is refused.
     *
     * @return OK on success, FAIL on error.
     */
    int pick_data_filt(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked);

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
    static MNERawData* open_file_comp(const std::filesystem::path& name, int omit_skip, int allow_maxshield, const MNEFilterDef& filter, int comp_set);

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
    static MNERawData* open_file(const std::filesystem::path& name, int omit_skip, int allow_maxshield, const MNEFilterDef& filter);

    //=========================================================================================================
    /**
     * Attaches channel derivations (MNE-C @c mne_raw_attach_derivations): merges them into deriv,
     * matches the merged set to the channels of the file into deriv_matched and validates it
     * against the channel info.
     *
     * @param[in] derivations    The derivations to add.
     * @param[in] keepPrevious   Keep the derivations attached before instead of replacing them.
     *
     * @return The number of derived channels that are valid for these data.
     */
    int attachDerivations(const MNEDerivSet& derivations, bool keepPrevious = false);

public:
    std::string filename;                         /**< Path to the raw FIFF file. */
    std::unique_ptr<mne::FiffStream> stream;      /**< Open FIFF stream for reading. */
    std::unique_ptr<mne::MNERawInfo> info;        /**< Raw data information loaded using MNE routines. */
    StringList ch_names;                          /**< Channel names as a flat list. */
    StringList badlist;                           /**< Bad channel names. */
    Eigen::VectorXi bad;                          /**< Boolean array marking bad channels (0 = good, 1 = bad). */
    std::vector<mne::MNERawBufDef> bufs;          /**< Raw data buffer definitions. */
    std::vector<mne::MNERawBufDef> filt_bufs;     /**< Filtered data buffer definitions. */
    int first_samp;                               /**< First sample index in the file. */
    int omit_samp;                                /**< Number of skip samples omitted from the beginning. */
    int first_samp_old;                           /**< First sample index (old-version compatible value). */
    int omit_samp_old;                            /**< Omitted samples (old-version compatible value). */
    int nsamp;                                    /**< Total number of samples in the file. */
    Eigen::VectorXf first_sample_val;             /**< Values at the first sample (for DC offset correction before filtering). */
    std::unique_ptr<mne::MNEProjOp> proj;         /**< SSP projection operator. */
    std::unique_ptr<mne::MNESssData> sss;         /**< SSS data found in this file. */
    std::unique_ptr<mne::MNECTFCompDataSet> comp; /**< CTF compensation data. */
    int comp_file;                                /**< Compensation grade stored in the file. */
    int comp_now;                                 /**< Current compensation grade applied to data. */
    std::unique_ptr<MNEFilterDef> filter;         /**< Filter definition (highpass/lowpass). */
    std::unique_ptr<FilterData> filter_data;      /**< Pre-computed frequency-domain filter state. */
    std::unique_ptr<MNEEventList> event_list;     /**< Trigger event list. */
    unsigned int max_event;                       /**< Maximum event number in use. */
    std::string dig_trigger;                      /**< Name of the digital trigger channel. */
    unsigned int dig_trigger_mask;                /**< Bit mask applied to digital trigger channel. */
    Eigen::VectorXf offsets;                      /**< DC offset corrections for display. */
    std::unique_ptr<RingBuffer> ring;             /**< Ring buffer for raw data. */
    std::unique_ptr<RingBuffer> filt_ring;        /**< Ring buffer for filtered data. */
    std::unique_ptr<mne::MNEDerivSet> deriv;      /**< Derivation data definitions. */
    std::unique_ptr<mne::MNEDeriv> deriv_matched; /**< Derivation data matched to this raw data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNERAWDATA_H
