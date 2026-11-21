//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     realtimehpiresult.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2020
 * @brief    Contains the declaration of the RealTimeHpiResult class.
 */

#ifndef REALTIMEHPIRESULT_H
#define REALTIMEHPIRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include <inv/hpi/inv_hpi_fit.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QColor>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
class FiffDigitizerData;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::scan
//=============================================================================================================

namespace mne::scan
{

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeHpiResult
 *
 * @brief The RealTimeHpiResult class provides a container for real-time HPI fitting results.
 */
class SCMEASSHARED_EXPORT RealTimeHpiResult : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeHpiResult> SPtr;            /**< Shared pointer type for RealTimeHpiResult. */
    typedef QSharedPointer<const RealTimeHpiResult> ConstSPtr; /**< Const shared pointer type for RealTimeHpiResult. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeHpiResult.
     */
    explicit RealTimeHpiResult(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeHpiResult.
     */
    virtual ~RealTimeHpiResult();

    //=========================================================================================================
    /**
     * Set the fiff info
     *
     * @param[in] pFiffInfo     the new fiff info.
     */
    void setFiffInfo(std::shared_ptr<mne::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Get the fiff info
     *
     * @return     the current fiff info.
     */
    std::shared_ptr<mne::FiffInfo> getFiffInfo();

    //=========================================================================================================
    /**
     * New covariance to distribute
     *
     * @param[in] v     the covariance which should be distributed.
     */
    virtual void setValue(const mne::HpiFitResult& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QSharedPointer<mne::HpiFitResult>& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeHpiResult contains values
     *
     * @return whether RealTimeHpiResult contains values.
     */
    inline bool isInitialized() const;

    //=========================================================================================================
    /**
     * Returns digitizer data for measurement
     *
     * @return the current set digitizer data
     */
    inline std::shared_ptr<mne::FiffDigitizerData> digitizerData() const;

    //=========================================================================================================
    /**
     * Sets digitizer data for measurement
     *
     * @param[in] digData   digitizer data from measurment
     */
    void setDigitizerData(std::shared_ptr<mne::FiffDigitizerData> digData);

    //=========================================================================================================
    /**
     * Head positions of all fits seen so far, oldest first.
     *
     * Each entry is the head origin in device coordinates, taken from the
     * translation of the device-to-head transform of one fit. Plotting the
     * entries as a connected line gives the path the subject's head travelled
     * over the course of the measurement.
     *
     * @return the recorded head positions in metres.
     */
    QVector<Eigen::Vector3f> headPositionHistory() const;

    //=========================================================================================================
    /**
     * Discard the recorded head positions, for example when starting a new run.
     */
    void clearHeadPositionHistory();

    //=========================================================================================================
    /**
     * Set how many head positions are kept.
     *
     * The history is a ring: once it is full the oldest entry is dropped. A
     * value of zero disables recording entirely.
     *
     * @param[in] iMaxPositions   maximum number of positions to keep.
     */
    void setHeadPositionHistorySize(int iMaxPositions);

private:
    mutable QMutex m_qMutex; /**< Mutex to ensure thread safety. */
    bool m_bInitialized;     /**< If values are stored.*/

    QSharedPointer<mne::HpiFitResult> m_pHpiFitResult;      /**< The HPI fit result. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;             /**< The Fiff Info. */
    std::shared_ptr<mne::FiffDigitizerData> m_pFiffDigData; /**< The Fiff Digigtizer Data */

    QVector<Eigen::Vector3f> m_vecHeadPositions; /**< Head origin in device coordinates for every fit so far, oldest first. */
    int m_iMaxHeadPositions;                     /**< Ring size of m_vecHeadPositions. Zero disables recording. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RealTimeHpiResult::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

inline std::shared_ptr<mne::FiffDigitizerData> RealTimeHpiResult::digitizerData() const
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffDigData;
}

} // NAMESPACE

#endif // REALTIMEHPIRESULT_H
