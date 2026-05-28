//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_evoked.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Single averaged evoked response: time axis, samples, baseline, channel info and processing history.
 *
 * @ref FiffEvoked is one averaged response as written under a
 * @c FIFFB_EVOKED block: the time axis (@c times, @c first, @c last),
 * the (nchan × nsamples) data matrix, the baseline interval, the
 * averaging kind (@c FIFFV_ASPECT_AVERAGE,
 * @c FIFFV_ASPECT_STD_ERR, ...), the trigger comment, the number of
 * averaged epochs (@c nave) and the associated @ref FiffInfo. Multiple
 * @ref FiffEvoked objects belonging to the same recording (one per
 * condition) make up a @ref FiffEvokedSet.
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVariant>
#include <QPair>
#include <QSharedPointer>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Single averaged evoked response: time axis, data, baseline, channel info and averaging metadata.
 *
 * Holds one @c FIFFB_EVOKED block: @c times, @c data, @c first /
 * @c last sample indices, @c baseline, @c kind, @c nave, @c comment and
 * the @ref FiffInfo describing the channels. Use @ref FiffEvokedSet for
 * recordings that contain multiple averaging conditions.
 */
class FIFFSHARED_EXPORT FiffEvoked
{
public:
    using SPtr = QSharedPointer<FiffEvoked>;            /**< Shared pointer type for FiffEvoked. */
    using ConstSPtr = QSharedPointer<const FiffEvoked>; /**< Const shared pointer type for FiffEvoked. */
    using UPtr = std::unique_ptr<FiffEvoked>;             /**< Unique pointer type for FiffEvoked. */
    using ConstUPtr = std::unique_ptr<const FiffEvoked>;  /**< Const unique pointer type for FiffEvoked. */

    //=========================================================================================================
    /**
     * Constructs a fiff evoked data.
     */
    FiffEvoked();

    //=========================================================================================================
    /**
     * Constructs fiff evoked data, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the evoked data set.
     * @param[in] setno          The set to pick. Dataset ID number (int) or comment/name (str). Optional if there isonly one data set in file.
     * @param[in] t_baseline     The time interval to apply rescaling / baseline correction. If None do not apply it. If baseline is (a, b).
     *                           the interval is between "a (s)" and "b (s)". If a is None the beginning of the data is used and if b is
     *                           None then b is set to the end of the interval. If baseline is equal ot (None, None) all the time interval is used.
     *                           If None, no correction is applied.
     * @param[in] proj           Apply SSP projection vectors (optional, default = true).
     * @param[in] p_aspect_kind  Either "FIFFV_ASPECT_AVERAGE" or "FIFFV_ASPECT_STD_ERR". The type of data to read. Only used if "setno" is a str.
     */
    FiffEvoked(QIODevice& p_IODevice,
               QVariant setno = 0,
               QPair<float,float> t_baseline = defaultFloatPair,
               bool proj = true,
               fiff_int_t p_aspect_kind = FIFFV_ASPECT_AVERAGE);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffEvoked    Fiff evoked data which should be copied.
     */
    FiffEvoked(const FiffEvoked& p_FiffEvoked);

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
    inline QStringList ch_names();

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
    inline QString aspectKindToString() const;

    //=========================================================================================================
    /**
     * Returns whether FiffEvoked is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool isEmpty() const;

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
    FiffEvoked pick_channels(const QStringList& include = defaultQStringList,
                             const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
     * fiff_read_evoked
     *
     *
     * Wrapper for the FiffEvokedDataSet::read_evoked static function
     *
     * Read one evoked data set
     *
     * @param[in] p_IODevice     An fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] p_FiffEvoked  The read evoked data.
     * @param[in] setno          the set to pick. Dataset ID number (int) or comment/name (str). Optional if there isonly one data set in file.
     * @param[in] t_baseline       The time interval to apply rescaling / baseline correction. If None do not apply it. If baseline is (a, b).
     *                           the interval is between "a (s)" and "b (s)". If a is None the beginning of the data is used and if b is
     *                           None then b is set to the end of the interval. If baseline is equal ot (None, None) all the time interval is used.
     *                           If None, no correction is applied.
     * @param[in] proj           Apply SSP projection vectors (optional, default = true).
     * @param[in] p_aspect_kind  Either "FIFFV_ASPECT_AVERAGE" or "FIFFV_ASPECT_STD_ERR". The type of data to read. Only used if "setno" is a str.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(QIODevice& p_IODevice,
                     FiffEvoked& p_FiffEvoked,
                     QVariant setno = 0, QPair<float,float> t_baseline = defaultFloatPair,
                     bool proj = true,
                     fiff_int_t p_aspect_kind = FIFFV_ASPECT_AVERAGE);

    //=========================================================================================================
    /**
     * Set a new fiff measurement info
     *
     * @param[in] p_info     Info to set.
     * @param[in] proj       Apply SSP projection vectors (optional, default = true).
     */
    void setInfo(const FiffInfo &p_info,
                 bool proj = true);

    //=========================================================================================================
    /**
     * Inputs a new data set and recalculates the average. This function also iterates the nave parameter by one.
     *
     * @param[in] newData     the new data set which is to be added to the current average.
     *
     * @return the updated FiffEvoked.
     */
    FiffEvoked & operator+=(const Eigen::MatrixXd &newData);

    //=========================================================================================================
    /**
     * Applies baseline correction to the evoked data.
     *
     * @param[in] p_baseline     time definition of the baseline in seconds [from, to].
     */
    void applyBaselineCorrection(QPair<float,float>& p_baseline);

public:
    FiffInfo                    info;               /**< Measurement info. */
    fiff_int_t                  nave;               /**< Number of averaged epochs. */
    fiff_int_t                  aspect_kind;        /**< Aspect identifier, either FIFFV_ASPECT_AVERAGE or FIFFV_ASPECT_STD_ERR. */
    fiff_int_t                  first;              /**< First time sample. */
    fiff_int_t                  last;               /**< Last time sample. */
    QString                     comment;            /**< Comment on dataset. Can be the condition. */
    Eigen::RowVectorXf          times;              /**< Vector of time instants in seconds. */
    Eigen::MatrixXd             data;               /**< 2D array of shape [n_channels x n_times]; Evoked response. */
    Eigen::MatrixXd             proj;               /**< SSP projection. */
    QPair<float,float>          baseline;           /**< Baseline information in seconds form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QStringList FiffEvoked::ch_names()
{
    return info.ch_names;
}

//=============================================================================================================

inline QString FiffEvoked::aspectKindToString() const
{
    if(aspect_kind == FIFFV_ASPECT_AVERAGE)
        return QString("Average");
    else if(aspect_kind == FIFFV_ASPECT_STD_ERR)
        return QString("Standard_error");
    else
        return QString("Unknown");
}

//=============================================================================================================

inline bool FiffEvoked::isEmpty() const
{
    return nave == -1;
}

} // NAMESPACE

#ifndef metatype_fiffevoked
#define metatype_fiffevoked
Q_DECLARE_METATYPE(FIFFLIB::FiffEvoked);/**< Provides QT META type declaration of the FIFFLIB::FiffEvoked type. For signal/slot and QVariant usage.*/
#endif

#ifndef metatype_fiffevokedsptr
#define metatype_fiffevokedsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffEvoked::SPtr);/**< Provides QT META type declaration of the FIFFLIB::FiffEvoked type. For signal/slot and QVariant usage.*/
#endif

#endif // FIFF_EVOKED_H
