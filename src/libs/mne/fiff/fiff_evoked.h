//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_evoked.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Single averaged evoked response: time axis, samples, baseline, channel info and processing history.
 *
 * @ref mne::FiffEvoked is one averaged response as written under a
 * @c FIFFB_EVOKED block: the time axis (@c times, @c first, @c last),
 * the (nchan × nsamples) data matrix, the baseline interval, the
 * averaging kind (@c FIFFV_ASPECT_AVERAGE,
 * @c FIFFV_ASPECT_STD_ERR, ...), the trigger comment, the number of
 * averaged epochs (@c nave) and the associated @ref mne::FiffInfo. Multiple
 * @ref mne::FiffEvoked objects belonging to the same recording (one per
 * condition) make up a @ref mne::FiffEvokedSet.
 *
 * Drop-in counterpart of @c mne.Evoked in MNE-Python: the same kind /
 * nave / comment / baseline semantics and the same channel-picking,
 * projector-application and SSP behaviour.
 */

#ifndef FIFF_EVOKED_H
#define FIFF_EVOKED_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_info.h"
#include "fiff_types.h"
#include "fiff_file.h"
#include <mne/base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Single averaged evoked response: time axis, data, baseline, channel info and averaging metadata.
 *
 * Holds one @c FIFFB_EVOKED block: @c times, @c data, @c first /
 * @c last sample indices, @c baseline, @c kind, @c nave, @c comment and
 * the @ref FiffInfo describing the channels. Use @ref FiffEvokedSet for
 * recordings that contain multiple averaging conditions.
 *
 * @snippet ex_fiff_api/main.cpp fiff_evoked_set_usage
 */
class FIFFSHARED_EXPORT FiffEvoked
{
public:
    //=========================================================================================================
    /**
     * Constructs a fiff evoked data.
     */
    FiffEvoked();

    /** Selects a data set by its index in the file or by its comment. */
    using Condition = std::variant<int, std::string>;

    /** Baseline interval [from, to] in seconds; equal bounds mean no correction. */
    using Baseline = std::optional<std::pair<float, float>>;

    //=========================================================================================================
    /**
     * Reads one evoked data set, like read(); throws mne::Error if it cannot.
     *
     * @param[in] path        The file.
     * @param[in] condition   Index of the data set, or its comment together with @p kind.
     * @param[in] baseline    Interval to subtract the mean of, or none for no baseline correction.
     * @param[in] proj        Apply the SSP projectors of the file.
     * @param[in] kind        FIFFV_ASPECT_AVERAGE or FIFFV_ASPECT_STD_ERR; selects among sets of the same comment.
     */
    explicit FiffEvoked(const std::filesystem::path& path,
                        const Condition& condition = 0,
                        const Baseline& baseline = {},
                        bool proj = true,
                        fiff_int_t kind = FIFFV_ASPECT_AVERAGE);

    //=========================================================================================================
    /**
     * Reads one evoked data set of a FIFF file, like mne.read_evokeds.
     *
     * @param[in] path        The file.
     * @param[in] condition   Index of the data set, or its comment together with @p kind.
     * @param[in] baseline    Interval to subtract the mean of, or none for no baseline correction.
     * @param[in] proj        Apply the SSP projectors of the file.
     * @param[in] kind        FIFFV_ASPECT_AVERAGE or FIFFV_ASPECT_STD_ERR; selects among sets of the same comment.
     *
     * @return The evoked data, or why the file holds none matching @p condition.
     */
    [[nodiscard]] static mne::Expected<FiffEvoked> read(const std::filesystem::path& path,
                                                        const Condition& condition = 0,
                                                        const Baseline& baseline = {},
                                                        bool proj = true,
                                                        fiff_int_t kind = FIFFV_ASPECT_AVERAGE);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffEvoked    Fiff evoked data which should be copied.
     */
    FiffEvoked(const FiffEvoked& p_FiffEvoked);

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
    FiffEvoked& operator=(const FiffEvoked& other) = default;

    //=========================================================================================================
    /**
     * Destroys the FiffEvoked.
     */
    ~FiffEvoked();

    //=========================================================================================================
    /**
     * Returns list of channel names stored in fiff info -> this is to stay consistent with python.
     *
     * @return List of channel names.
     */
    inline const StringList& ch_names() const;

    //=========================================================================================================
    /**
     * Initializes fiff evoked data.
     */
    void clear();

    //=========================================================================================================
    /**
     * Provides the python Evoked string formatted aspect_kind, which is stored in kind:
     * "average" <-> FIFFV_ASPECT_AVERAGE, "standard_error" <-> FIFFV_ASPECT_STD_ERR or "unknown"
     *
     * @return string formatted aspect_kind.
     */
    inline std::string aspectKindToString() const;

    //=========================================================================================================
    /**
     * Returns whether FiffEvoked is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * fiff_pick_channels_evoked
     *
     *
     * Pick desired channels from evoked-response data
     *
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the desired fiff evoked data.
     */
    FiffEvoked pick_channels(const StringList& include = {},
                             const StringList& exclude = {}) const;

    //=========================================================================================================
    /**
     * Set a new fiff measurement info
     *
     * @param[in] p_info     Info to set.
     * @param[in] applyProj       Apply SSP projection vectors (optional, default = true).
     */
    void setInfo(const FiffInfo& p_info,
                 bool applyProj = true);

    //=========================================================================================================
    /**
     * Inputs a new data set and recalculates the average. This function also iterates the nave parameter by one.
     *
     * @param[in] newData     the new data set which is to be added to the current average.
     *
     * @return the updated FiffEvoked.
     */
    FiffEvoked& operator+=(const Eigen::MatrixXd& newData);

    /** Predefined weighting strategies of combine(). */
    enum class CombineWeights
    {
        Nave, /**< Sum-to-one weights proportional to each nave. */
        Equal /**< Every data set is weighted by 1 / number of data sets. */
    };

    //=========================================================================================================
    /**
     * Merges evoked data by weighted addition or subtraction (mne.combine_evoked).
     *
     * All data sets must have the same channels and time instants. The result carries
     * the union of the bad channels, the weighted sum of the data, a comment such as
     * "0.474 × A + 0.526 × B" and nave = 1 / sum(w_i^2 / nave_i), rounded to an
     * integer as required by the FIFF format.
     *
     * @param[in] evokeds    The evoked data sets.
     * @param[in] weights    One weight per data set.
     *
     * @return The combined evoked data, or an empty FiffEvoked if the inputs are inconsistent.
     */
    static FiffEvoked combine(const std::vector<FiffEvoked>& evokeds, const Eigen::VectorXd& weights);

    //=========================================================================================================
    /**
     * Merges evoked data with a predefined weighting strategy (weights="nave" or "equal").
     *
     * @param[in] evokeds    The evoked data sets.
     * @param[in] weights    The weighting strategy.
     *
     * @return The combined evoked data, or an empty FiffEvoked if the inputs are inconsistent.
     */
    static FiffEvoked combine(const std::vector<FiffEvoked>& evokeds, CombineWeights weights);

    //=========================================================================================================
    /**
     * Applies baseline correction to the evoked data.
     *
     * @param[in] p_baseline     time definition of the baseline in seconds [from, to].
     */
    void applyBaselineCorrection(std::pair<float, float>& p_baseline);

public:
    FiffInfo info;                    /**< Measurement info. */
    fiff_int_t nave;                  /**< Number of averaged epochs. */
    fiff_int_t aspect_kind;           /**< Aspect identifier, either FIFFV_ASPECT_AVERAGE or FIFFV_ASPECT_STD_ERR. */
    fiff_int_t first;                 /**< First time sample. */
    fiff_int_t last;                  /**< Last time sample. */
    std::string comment;              /**< Comment on dataset. Can be the condition. */
    Eigen::RowVectorXf times;         /**< Vector of time instants in seconds. */
    Eigen::MatrixXd data;             /**< 2D array of shape [n_channels x n_times]; Evoked response. */
    Eigen::MatrixXd proj;             /**< SSP projection. */
    std::pair<float, float> baseline; /**< Baseline information in seconds form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const StringList& FiffEvoked::ch_names() const
{
    return info.ch_names;
}

//=============================================================================================================

inline std::string FiffEvoked::aspectKindToString() const
{
    if (aspect_kind == FIFFV_ASPECT_AVERAGE)
        return "Average";
    else if (aspect_kind == FIFFV_ASPECT_STD_ERR)
        return "Standard_error";
    else
        return "Unknown";
}

//=============================================================================================================

inline bool FiffEvoked::empty() const
{
    return nave == -1;
}

} // NAMESPACE

#endif // FIFF_EVOKED_H
