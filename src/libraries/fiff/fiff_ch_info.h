//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_ch_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    FIFF channel descriptor record (FIFF_CH_INFO): per-channel logical/scanner numbers, kind, calibration, coil type, unit, location and orientation.
 *
 * A @c FIFF_CH_INFO tag stores everything needed to interpret one column
 * of the raw / evoked data matrix: which physical channel produced it
 * (@c scanno / @c logno), what kind of sensor it is
 * (@c FIFFV_MEG_CH / @c FIFFV_EEG_CH / @c FIFFV_STIM_CH / ...), the
 * calibration constants @c cal and @c range that map ADC counts to SI
 * units, the coil-frame transform encoded in the @ref FiffChPos sub-record,
 * the coil type (@c FIFFV_COIL_VV_PLANAR_T1, @c FIFFV_COIL_VV_MAG_T1,
 * @c FIFFV_COIL_CTF_GRAD, ...) and the SI unit + multiplier of the
 * calibrated samples.
 *
 * @ref FiffChInfo wraps that record. Exact field-for-field parity with the
 * @c ch_info dict consumed by @c mne.io.meas_info in MNE-Python is
 * mandatory: forward models, source localizers and CTF compensators all
 * key on these fields and silently produce wrong topographies if any
 * field drifts.
 */

#ifndef FIFF_CH_INFO_H
#define FIFF_CH_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_ch_pos.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Per-channel FIFF descriptor: identifiers, kind, calibration, coil type, channel-frame coil position and SI unit.
 *
 * Holds the exact field set of the on-disk @c fiffChInfoRec: @c scanno,
 * @c logno, @c kind, @c range, @c cal, @c coil_type, the embedded
 * @ref FiffChPos coil location, @c unit and @c unit_mul. The @c ch_name
 * string lives next to the record because the FIFF stream stores it as a
 * separate @c FIFF_CH_NAME tag in modern files.
 */
class FIFFSHARED_EXPORT FiffChInfo
{
public:
    using SPtr = QSharedPointer<FiffChInfo>;            /**< Shared pointer type for FiffChInfo. */
    using ConstSPtr = QSharedPointer<const FiffChInfo>; /**< Const shared pointer type for FiffChInfo. */
    using UPtr = std::unique_ptr<FiffChInfo>;             /**< Unique pointer type for FiffChInfo. */
    using ConstUPtr = std::unique_ptr<const FiffChInfo>;  /**< Const unique pointer type for FiffChInfo. */

    //=========================================================================================================
    /**
     * Constructs the channel info descriptor.
     */
    FiffChInfo();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffChInfo   Channel Info descriptor which should be copied.
     */
    FiffChInfo(const FiffChInfo &p_FiffChInfo);

    //=========================================================================================================
    /**
     * Destroys the channel info descriptor.
     */
    ~FiffChInfo();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffChInfoRec) 20*int + 16 = 20*4 + 16 = 96
     *
     * @return the size of the old struct fiffChInfoRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * Check that all EEG channels in the list have reasonable (non-origin) locations.
     * Formerly mne_check_chinfo (MNE C).
     *
     * @param[in] chs  The list of channel info structures to check.
     * @param[in] nch  The number of channels to check.
     * @return true if all EEG channels have valid locations, false otherwise.
     */
    static bool checkEegLocations(const QList<FiffChInfo>& chs, int nch);

    //=========================================================================================================
    /**
     * Check whether this channel has a valid EEG electrode position.
     *
     * A channel is a valid EEG channel if its kind is FIFFV_EEG_CH, its
     * electrode position is not at the origin, and its coil type is not FIFFV_COIL_NONE.
     *
     * @return true if this is a valid EEG channel with proper electrode location.
     */
    bool isValidEeg() const;

    //=========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffChInfo &a, const FiffChInfo &b);

public:
    fiff_int_t    scanNo;       /**< Scanning order number. */
    fiff_int_t    logNo;        /**< Logical channel #. */
    fiff_int_t    kind;         /**< Kind of channel. */
    fiff_float_t  range;        /**< Voltmeter range (-1 = auto ranging). */
    fiff_float_t  cal;          /**< Calibration from volts to units used. */
    FiffChPos     chpos;        /**< Channel location. */
    fiff_int_t    unit;         /**< Unit of measurement. */
    fiff_int_t    unit_mul;     /**< Unit multiplier exponent. */
    QString       ch_name;      /**< Descriptive name for the channel. */

    //Convenience members - MATLAB -
    Eigen::Matrix<float,4,4, Eigen::DontAlign>    coil_trans;     /**< Coil coordinate system transformation. */
    Eigen::Matrix<float,3,2, Eigen::DontAlign>    eeg_loc;        /**< Channel location. */
    fiff_int_t    coord_frame;                      /**< Coordinate Frame. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffChInfo::storageSize()
{
    // On-disk layout: scanNo, logNo, kind, range, cal, chpos, unit, unit_mul, ch_name[16]
    // (C++ class uses QString but on-disk stores fixed 16-char name)
    return 3 * sizeof(fiff_int_t) + 2 * sizeof(fiff_float_t)
         + FiffChPos::storageSize() + 2 * sizeof(fiff_int_t) + 16;
}

//=============================================================================================================

inline bool operator== (const FiffChInfo &a, const FiffChInfo &b)
{
    return (a.scanNo == b.scanNo &&
            a.logNo == b.logNo &&
            a.kind == b.kind &&
            a.cal == b.cal &&
            a.chpos == b.chpos &&
            a.unit == b.unit &&
            a.unit_mul == b.unit_mul &&
            a.coil_trans.isApprox(b.coil_trans, 0.0001f) &&
            a.eeg_loc.isApprox(b.eeg_loc, 0.0001f) &&
            a.coord_frame == b.coord_frame);
}
} // NAMESPACE

#endif // FIFF_CH_INFO_H
