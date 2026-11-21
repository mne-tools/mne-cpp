//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_info_base.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     February 2013
 * @brief    Minimal measurement-info subset (channel list, sampling rate, basic transforms) shared by FIFF readers that do not need the full FiffInfo.
 *
 * @ref mne::FiffInfoBase carries only what every downstream component needs to
 * interpret a data matrix: the channel list (@c chs / @c ch_names /
 * @c nchan), the sampling frequency, the @c FIFFV_COORD_DEVICE →
 * @c FIFFV_COORD_HEAD transform recovered from HPI, and the bad-channel
 * list. @ref mne::FiffInfo derives from it and adds the acquisition-specific
 * metadata (projectors, CTF compensators, filter setups, subject info,
 * HPI fit details, ...).
 *
 * Splitting the data this way lets stripped-down forms (e.g. evoked
 * fragments stored without acquisition metadata, or info subsets sent
 * over the real-time wire protocol) be passed around without forcing
 * every consumer to deal with optional fields. Mirrors the
 * @c mne.io.meas_info.MeasInfo / ``info_subset`` split in MNE-Python.
 */

#ifndef FIFF_INFO_BASE_H
#define FIFF_INFO_BASE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_ch_info.h"
#include "fiff_dig_point.h"
#include "fiff_ctf_comp.h"
#include "fiff_coord_trans.h"
#include "fiff_proj.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Stripped FIFF measurement info: channel list, sampling rate, device→head transform and bad-channel list.
 *
 * Owns just the fields needed to interpret a data matrix: @c chs,
 * @c ch_names, @c nchan, @c sfreq, @c bads, @c dev_head_t, @c ctf_head_t.
 * Base class of @ref FiffInfo, used directly when the rest of the
 * acquisition metadata is not available (e.g. realtime client streams,
 * trimmed evoked files).
 *
 * @snippet ex_fiff_api/main.cpp fiff_info_usage
 */
class FIFFSHARED_EXPORT FiffInfoBase
{
public:
    //=========================================================================================================
    /**
     * Constructors the light fiff measurement file information.
     */
    FiffInfoBase();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffInfoBase  light FIFF measurement information which should be copied.
     */
    FiffInfoBase(const FiffInfoBase& p_FiffInfoBase);

    //=========================================================================================================
    /**
     * Copy assignment. Every member has value semantics, so the
     * compiler-generated member-wise assignment is correct. It is declared
     * explicitly because declaring a copy constructor and/or a destructor
     * deprecates the implicitly generated one.
     *
     * @param[in] other   Object to assign from.
     *
     * @return reference to this object.
     */
    FiffInfoBase& operator=(const FiffInfoBase& other) = default;

    //=========================================================================================================
    /**
     * Destroys the light fiff measurement file information.
     */
    virtual ~FiffInfoBase();

    //=========================================================================================================
    /**
     * Initializes light FIFF measurement information.
     */
    virtual void clear();

    //=========================================================================================================
    /**
     * Get channel type.
     *
     * @param[in] idx    Index of channel.
     *
     * @return Type of channel as in mne.channel_type ('grad', 'mag', 'eeg', 'seeg', 'ecog', 'hbo', 'eyegaze', ...).
     */
    std::string channel_type(std::int32_t idx) const;

    //=========================================================================================================
    /**
     * @param[in] name   Channel name.
     *
     * @return Index of the channel in ch_names, or -1 when there is none.
     */
    int channelIndex(std::string_view name) const;

    //=========================================================================================================
    /**
     * @param[in] name   Channel name.
     *
     * @return True when the channel is marked bad.
     */
    bool isBad(std::string_view name) const;

    //=========================================================================================================
    /**
     * Channel indices of each channel type, like mne.channel_indices_by_type.
     *
     * @param[in] picks  Channels to classify (empty: all).
     *
     * @return Every channel type mapped to the indices of its channels (empty lists for absent types).
     */
    std::map<std::string, std::vector<int>> channel_indices_by_type(const std::vector<int>& picks = {}) const;

    //=========================================================================================================
    /**
     * The data channels (MEG including reference MEG, EEG, CSD, sEEG, ECoG, DBS, fNIRS), like mne's picks="data".
     *
     * @param[in] exclude  Channel names to leave out, e.g. the bads.
     *
     * @return The channel indices in channel order.
     */
    Eigen::RowVectorXi pick_data_channels(const StringList& exclude = {}) const;

    //=========================================================================================================
    /**
     * Indices of the channel names that match a regular expression from their start, like mne.pick_channels_regexp.
     *
     * @param[in] ch_names  The channel names.
     * @param[in] regexp    The regular expression (Perl syntax).
     *
     * @return The selector (row vector).
     */
    static Eigen::RowVectorXi pick_channels_regexp(const StringList& ch_names, const std::string& regexp);

    //=========================================================================================================
    /**
     * True if FIFF measurement file information is empty.
     *
     * @return true if FIFF measurement file information is empty.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * fiff_pick_channels
     *
     * Make a selector to pick desired channels from data
     *
     * @param[in] ch_names  - The channel name list to consult.
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row Vector).
     */
    static Eigen::RowVectorXi pick_channels(const StringList& ch_names,
                                            const StringList& include = {},
                                            const StringList& exclude = {});

    //=========================================================================================================
    /**
     * fiff_pick_info
     *
     * Pick desired channels from measurement info
     *
     * @param[in] sel    List of channels to select.
     *
     * @return Info modified according to sel.
     */
    FiffInfoBase pick_info(const Eigen::RowVectorXi* sel = nullptr) const;

    //=========================================================================================================
    /**
     * fiff_pick_types (highy diversity in meg picking)
     *
     * Create a selector to pick desired channel types from data
     *
     * @param[in] meg        It can be "all", to select all or it can be "mag" or "grad" to select only gradiometers or magnetometers.
     * @param[in] eeg        Include EEG channels.
     * @param[in] stim       Include stimulus channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row vector).
     */
    Eigen::RowVectorXi pick_types(const std::string& meg,
                                  bool eeg = false,
                                  bool stim = false,
                                  const StringList& include = {},
                                  const StringList& exclude = {}) const;

    // Without this, a literal such as "mag" converts to bool and selects the overload below.
    Eigen::RowVectorXi pick_types(const char* meg,
                                  bool eeg = false,
                                  bool stim = false,
                                  const StringList& include = {},
                                  const StringList& exclude = {}) const
    {
        return pick_types(std::string(meg), eeg, stim, include, exclude);
    }

    //=========================================================================================================
    /**
     * fiff_pick_types
     *
     * Create a selector to pick desired channel types from data
     * Use overloaded pick_types method to specify meg (grad, mag, ref_meg)type
     *
     * @param[in] meg        Include MEG channels.
     * @param[in] eeg        Include EEG channels.
     * @param[in] stim       Include stimulus channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row vector).
     */
    Eigen::RowVectorXi pick_types(bool meg,
                                  bool eeg = false,
                                  bool stim = false,
                                  const StringList& include = {},
                                  const StringList& exclude = {}) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand measurement info.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const FiffInfoBase& a, const FiffInfoBase& b);

    /**
     * Read MEG, compensation, and EEG channel information from this measurement info.
     *
     * Classifies channels by kind (FIFFV_MEG_CH, FIFFV_REF_MEG_CH, FIFFV_EEG_CH)
     * and returns the device-to-head transform and measurement ID.
     *
     * @param[out] megp         List of MEG channel info descriptors.
     * @param[out] nmegp        Number of MEG channels found.
     * @param[out] meg_compp    List of MEG compensation (reference) channel info descriptors.
     * @param[out] nmeg_compp   Number of compensation channels found.
     * @param[out] eegp         List of EEG channel info descriptors.
     * @param[out] neegp        Number of EEG channels found.
     * @param[out] meg_head_t   Device-to-head coordinate transformation.
     * @param[out] idp          Measurement ID.
     */
    void mne_read_meg_comp_eeg_ch_info(std::vector<FiffChInfo>& megp,
                                       int& nmegp,
                                       std::vector<FiffChInfo>& meg_compp,
                                       int& nmeg_compp,
                                       std::vector<FiffChInfo>& eegp,
                                       int& neegp,
                                       FiffCoordTrans& meg_head_t,
                                       FiffId& idp) const;

    /**
     * Parses the channel info information and returns a string list of channel types.
     *
     * @return The distinct channel types in channel order, as channel_type() names them (unknown kinds skipped).
     */
    StringList get_channel_types();

    //=========================================================================================================
    /**
     * Read bad channel names from a plain text file (one name per line, '#' comments skipped).
     *
     * @param[in]  name    Path to the bad channel file (empty string is a no-op).
     * @param[out] listOut The bad channel names read from the file.
     * @return true on success, false on error.
     */
    static bool readBadChannelsFromFile(const std::string& name, StringList& listOut);

public:
    std::string filename;                        /**< Filename when the info is read of a fiff file. */
    StringList bads;                             /**< List of bad channels. */
    FiffId meas_id;                              /**< Measurement ID. */
    fiff_int_t nchan;                            /**< Number of channels. */
    std::vector<FiffChInfo> chs;                 /**< List of all channel info descriptors. */
    StringList ch_names;                         /**< List of all channel names. */
    FiffCoordTrans dev_head_t;                   /**< Device to head coordinate transformation. */
    FiffCoordTrans ctf_head_t;                   /**< CTF to head coordinate transformation. */
    std::vector<FiffCoordTrans> all_coord_trans; /**< All coordinate transformations stored in the file, in file order. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffInfoBase::empty() const
{
    return this->nchan <= 0;
}

//=============================================================================================================

inline bool operator==(const FiffInfoBase& a, const FiffInfoBase& b)
{
    return (a.bads == b.bads &&
            //a.meas_id == b.meas_id &&
            a.nchan == b.nchan &&
            a.chs == b.chs &&
            a.ch_names == b.ch_names &&
            a.dev_head_t == b.dev_head_t &&
            a.ctf_head_t == b.ctf_head_t);
}
} // NAMESPACE

#endif // FIFF_INFO_BASE_H
