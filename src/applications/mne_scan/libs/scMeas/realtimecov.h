//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimecov.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RealTimeCov class.
 */

#ifndef REALTIMECOV_H
#define REALTIMECOV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include <memory>
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_cov.h>

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
// DEFINE NAMESPACE mne::scan
//=============================================================================================================

namespace mne::scan
{

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeCov
 *
 * @brief The RealTimeCov class provides a container for real-time covariance estimations.
 */
class SCMEASSHARED_EXPORT RealTimeCov : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeCov> SPtr;            /**< Shared pointer type for RealTimeCov. */
    typedef QSharedPointer<const RealTimeCov> ConstSPtr; /**< Const shared pointer type for RealTimeCov. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeCov.
     */
    explicit RealTimeCov(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeCov.
     */
    virtual ~RealTimeCov();

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
    virtual void setValue(const mne::FiffCov& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual std::shared_ptr<mne::FiffCov>& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeCov contains values
     *
     * @return whether RealTimeCov contains values.
     */
    inline bool isInitialized() const;

private:
    mutable QMutex m_qMutex; /**< Mutex to ensure thread safety. */

    std::shared_ptr<mne::FiffCov> m_pFiffCov;   /**< Covariance data set. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< The Fiff Info. */

    bool m_bInitialized; /**< If values are stored.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RealTimeCov::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}
} // NAMESPACE

#endif // REALTIMECOV_H
