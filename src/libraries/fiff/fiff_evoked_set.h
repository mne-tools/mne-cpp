//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_evoked_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Set of averaged evoked responses sharing a FiffInfo, plus the ave-style category / rejection descriptors for batch averaging.
 *
 * A single ``*-ave.fif`` typically contains several @c FIFFB_EVOKED
 * blocks (one per stimulus condition / averaging category). @ref
 * mne::FiffEvokedSet groups them: a shared @ref mne::FiffInfo and a list of
 * @ref mne::FiffEvoked instances, plus channel-picking and compensation
 * helpers that apply uniformly to the whole set.
 *
 * The header additionally ports the three batch-averaging descriptor
 * structs from MNE-C's @c browser_types.h (@ref mne::RejectionParams,
 * @ref mne::AverageCategory, @ref mne::AverageDescription) so the @c mne_browse_raw
 * "ave description" files can be parsed and consumed verbatim. Together
 * they describe artifact-rejection thresholds, per-category timing and
 * trigger logic, and the file-level output settings used by the
 * @c mne_process_raw averaging pipeline.
 */

#ifndef FIFF_EVOKED_SET_H
#define FIFF_EVOKED_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"
#include "fiff_evoked.h"
#include "fiff_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <filesystem>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffRawData;

//=============================================================================================================
/**
 * @brief Artifact-rejection thresholds for the MNE-C batch averaging pipeline (gradiometer / magnetometer / EEG / EOG / ECG, peak-to-peak and flatness).
 *
 * Field-for-field port of @c rejDataRec from MNE-C @c browser_types.h.
 * Used by @ref AverageDescription to drive @c mne_process_raw's
 * peak-to-peak and flatness rejection when computing batch evokeds; the
 * default values match the MNE-C defaults so existing "ave description"
 * files keep producing identical results.
 *
 * @snippet ex_mne_api/main.cpp mne_description_parser_usage
 */
struct FIFFSHARED_EXPORT RejectionParams
{
    float stimIgnore = 0.0f;         /**< Ignore this many seconds around the stimulus. */
    float megGradReject = 2000e-13f; /**< Gradiometer rejection (T/m). */
    float megMagReject = 3e-12f;     /**< Magnetometer rejection (T). */
    float eegReject = 100e-6f;       /**< EEG rejection (V). */
    float eogReject = 150e-6f;       /**< EOG rejection (V). */
    float ecgReject = 0.0f;          /**< ECG rejection (V). */
    float megGradFlat = 0.0f;        /**< Gradiometer flatness (T/m). */
    float megMagFlat = 0.0f;         /**< Magnetometer flatness (T). */
    float eegFlat = 0.0f;            /**< EEG flatness (V). */
    float eogFlat = 0.0f;            /**< EOG flatness (V). */
    float ecgFlat = 0.0f;            /**< ECG flatness (V). */
};

//=============================================================================================================
/**
 * @brief One averaging category in an MNE-C ave-description file: trigger logic, timing window, baseline interval and display color.
 *
 * Port of @c aveCategRec from MNE-C @c browser_types.h. Describes one
 * condition the batch averager produces: which trigger event (with
 * required previous / following events and ignore masks), the epoch time
 * window (@c tmin / @c tmax), the optional baseline interval, and the
 * post-processing flags (baseline correction, std-error vs.\ average,
 * absolute values).
 *
 * @snippet ex_mne_api/main.cpp mne_description_parser_usage
 */
struct FIFFSHARED_EXPORT AverageCategory
{
    std::string comment;              /**< Description. */
    std::vector<unsigned int> events; /**< The interesting events. */
    unsigned int nextEvent = 0;       /**< Require this event next. */
    unsigned int prevEvent = 0;       /**< Require this event just before. */
    unsigned int ignore = 0;          /**< Which trigger lines to ignore. */
    unsigned int prevIgnore = 0;      /**< Ignore mask for previous event. */
    unsigned int nextIgnore = 0;      /**< Ignore mask for next event. */
    float delay = 0.0f;               /**< Stimulus delay (s). */
    float tmin = -0.2f;               /**< Minimum time (s). */
    float tmax = 0.5f;                /**< Maximum time (s). */
    float bmin = 0.0f;                /**< Baseline min (s). */
    float bmax = 0.0f;                /**< Baseline max (s). */
    bool doBaseline = false;          /**< Should we baseline? */
    bool doStdErr = false;            /**< Compute std error of mean? */
    bool doAbs = false;               /**< Compute absolute values? */
    float color[3] = {0, 0, 0};       /**< Display color (unused in batch). */
};

//=============================================================================================================
/**
 * @brief Top-level MNE-C ave-description record: comment, category list, shared rejection limits and output file paths.
 *
 * Port of @c aveDataRec from MNE-C @c browser_types.h. Owns the list of
 * @ref AverageCategory entries, the shared @ref RejectionParams, the
 * skew-correction flag and the I/O paths (output ave file, event source,
 * log file) so an entire batch-averaging job can be described as one
 * value.
 *
 * @snippet ex_mne_api/main.cpp mne_description_parser_usage
 */
struct FIFFSHARED_EXPORT AverageDescription
{
    std::string comment;                     /**< Description. */
    std::vector<AverageCategory> categories; /**< The categories. */
    RejectionParams rej;                     /**< Rejection limits. */
    bool fixSkew = false;                    /**< Fix skew on trigger lines. */
    std::string filename;                    /**< Output file. */
    std::string eventFile;                   /**< Read events from here. */
    std::string logFile;                     /**< Save log here. */
};

//=============================================================================================================
/**
 * @brief Set of @ref FiffEvoked instances sharing one @ref FiffInfo, plus channel-picking and compensation helpers.
 *
 * The on-disk shape of a ``*-ave.fif``: one @ref FiffInfo and one
 * @ref FiffEvoked per @c FIFFB_EVOKED block (one per averaging
 * condition). Conversion utilities pick channels and shift compensation
 * state across the whole set in lock-step.
 *
 * @snippet ex_fiff_api/main.cpp fiff_evoked_set_usage
 */
class FIFFSHARED_EXPORT FiffEvokedSet
{
public:
    //=========================================================================================================
    /**
     * Constructs a fiff evoked data set.
     */
    FiffEvokedSet();

    //=========================================================================================================
    /**
     * Reads evoked data sets of a FIFF file, like read(); throws mne::Error if it cannot.
     *
     * @param[in] path         The file.
     * @param[in] conditions   Indices or comments of the averages to read, in this order; empty reads all.
     * @param[in] baseline     Interval to subtract the mean of, or none for no baseline correction.
     * @param[in] proj         Apply the SSP projectors of the file.
     */
    explicit FiffEvokedSet(const std::filesystem::path& path,
                           const std::vector<FiffEvoked::Condition>& conditions = {},
                           const FiffEvoked::Baseline& baseline = {},
                           bool proj = true);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffEvokedSet    Fiff evoked data set which should be copied.
     */
    FiffEvokedSet(const FiffEvokedSet& p_FiffEvokedSet);

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
    FiffEvokedSet& operator=(const FiffEvokedSet& other) = default;

    //=========================================================================================================
    /**
     * Destroys the fiff evoked data set.
     */
    ~FiffEvokedSet();

    //=========================================================================================================
    /**
     * Initializes fiff evoked data set.
     */
    void clear();

    //=========================================================================================================
    /**
     * fiff_pick_channels_evoked
     *
     * Pick desired channels from evoked-response data
     *
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the desired fiff evoked data set.
     */
    FiffEvokedSet pick_channels(const StringList& include = {},
                                const StringList& exclude = {}) const;

    //=========================================================================================================
    /**
     * mne_compensate_to
     *
     * Changes the CTF gradient compensation grade of all evoked responses in place.
     *
     * @param[in] to    Desired compensation grade.
     *
     * @return true if the data were changed, false if they already had that grade or it is not available.
     */
    bool compensate_to(fiff_int_t to);

    //=========================================================================================================
    /**
     * fiff_find_evoked
     *
     * Find evoked data sets
     *
     * @param[out] p_FiffEvokedSet   The read evoked data set.
     *
     * @return true when any set was found, false otherwise.
     */
    bool find_evoked(const FiffEvokedSet& p_FiffEvokedSet) const;

    //=========================================================================================================
    /**
     * Reads evoked data sets of a FIFF file in one pass, like mne.read_evokeds.
     *
     * @param[in] path         The file.
     * @param[in] conditions   Indices or comments of the averages to read, in this order; empty reads all.
     * @param[in] baseline     Interval to subtract the mean of, or none for no baseline correction.
     * @param[in] proj         Apply the SSP projectors of the file.
     *
     * @return The evoked data sets, or why the file does not hold them.
     */
    [[nodiscard]] static mne::Expected<FiffEvokedSet> read(const std::filesystem::path& path,
                                                           const std::vector<FiffEvoked::Condition>& conditions = {},
                                                           const FiffEvoked::Baseline& baseline = {},
                                                           bool proj = true);

    //=========================================================================================================
    /**
     * Writes the evoked data sets to a FIFF file, like mne.write_evokeds.
     *
     * @param[in] path  The file.
     *
     * @return Nothing, or why the file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> save(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Compute a grand average across multiple evoked data sets (mne.grand_average with
     * interpolate_bads=False, drop_bads=True).
     * For each category, channels marked bad in any set are dropped, the common channels
     * are kept and the sets are combined with equal weights. nave is set to the number
     * of sets and the comment to "Grand average (n = N)".
     *
     * @param[in] evokedSets    List of evoked data sets to combine.
     * @return The grand-average evoked set, or an empty set if input is empty.
     */
    static FiffEvokedSet computeGrandAverage(const std::vector<FiffEvokedSet>& evokedSets);

    //=========================================================================================================
    /**
     * Compute epoch-based averages from raw data according to an averaging description.
     *
     * For each category in the description, matching events are epoched from the raw data,
     * artifact-rejected, baseline-corrected, and averaged into a FiffEvoked entry.
     *
     * @param[in] raw           The raw data.
     * @param[in] desc          The averaging description (categories, rejection, etc.).
     * @param[in] events        Event matrix (nEvents x 3): [sample, from, to].
     * @param[out] log          Processing log output.
     * @return FiffEvokedSet containing one FiffEvoked per category, or empty set on failure.
     */
    static FiffEvokedSet computeAverages(const FiffRawData& raw,
                                         const AverageDescription& desc,
                                         const Eigen::MatrixXi& events,
                                         std::string& log);

    //=========================================================================================================
    /**
     * Check whether an epoch passes artifact rejection criteria.
     *
     * @param[in] epoch     Epoch data (nChannels x nSamples).
     * @param[in] info      Channel info.
     * @param[in] bads      List of bad channel names.
     * @param[in] rej       Rejection parameters.
     * @param[out] reason   Rejection reason string (set only when rejected).
     * @return true if epoch is clean (not rejected).
     */
    static bool checkArtifacts(const Eigen::MatrixXd& epoch,
                               const FiffInfo& info,
                               const StringList& bads,
                               const RejectionParams& rej,
                               std::string& reason);

    /**
     * @brief Subtract baseline from each channel of an epoch.
     *
     * For each row (channel), the mean of the samples in [bminSamp, bminSamp+nBase)
     * is subtracted.
     *
     * @param[in,out] epoch     Data matrix (channels x samples) to correct in-place.
     * @param[in] bminSamp      First baseline sample (clamped to 0).
     * @param[in] bmaxSamp      Last baseline sample (clamped to epoch length - 1).
     */
    static void subtractBaseline(Eigen::MatrixXd& epoch, int bminSamp, int bmaxSamp);

public:
    FiffInfo info;                  /**< FIFF measurement information. */
    std::vector<FiffEvoked> evoked; /**< List of Fiff Evoked Data. */
};
} // NAMESPACE

#endif // FIFF_EVOKED_SET_H
