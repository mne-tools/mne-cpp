//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     realtimefwdsolution.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 * @brief    Contains the declaration of the RealTimeFwdSolution class.
 */

#ifndef REALTIMEFWDRESULT_H
#define REALTIMEFWDRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include <memory>
#include "measurement.h"

#include <mne/mne_forward_solution.h>

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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

namespace mne
{
class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::scan
//=============================================================================================================

namespace mne::scan
{

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeFwdSolution
 *
 * @brief The RealTimeFwdSolution class provides a container for real-time HPI fitting results.
 */
class SCMEASSHARED_EXPORT RealTimeFwdSolution : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeFwdSolution> SPtr;            /**< Shared pointer type for RealTimeFwdSolution. */
    typedef QSharedPointer<const RealTimeFwdSolution> ConstSPtr; /**< Const shared pointer type for RealTimeFwdSolution. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeFwdSolution.
     */
    explicit RealTimeFwdSolution(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeFwdSolution.
     */
    virtual ~RealTimeFwdSolution();

    //=========================================================================================================
    /**
     * Set the fiff info
     *
     * @param[in] pFiffInfo     The new fiff info.
     */
    void setFiffInfo(std::shared_ptr<mne::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Get the fiff info
     *
     * @return     The current fiff info.
     */
    std::shared_ptr<mne::FiffInfo> getFiffInfo();

    //=========================================================================================================
    /**
     * New forward solution to distribute
     *
     * @param[in] fwdSolution     The forward solution which should be distributed.
     */
    virtual void setValue(const std::shared_ptr<mne::MNEForwardSolution>& pFwdSolution);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return The last attached value.
     */
    virtual std::shared_ptr<mne::MNEForwardSolution> getValue();

    //=========================================================================================================
    /**
     * Returns if the fwd solution is clustered.
     *
     * @return if Forward is Clustered.
     */
    inline bool isClustered();

    //=========================================================================================================
    /**
     * Returns whether RealTimeFwdSolution contains values
     *
     * @return whether RealTimeFwdSolution contains values.
     */
    inline bool isInitialized() const;

private:
    mutable QMutex m_qMutex; /**< Mutex to ensure thread safety. */
    bool m_bInitialized;     /**< If values are stored.*/
    bool m_bClustered;       /**< If fwd is clustered.*/

    std::shared_ptr<mne::MNEForwardSolution> m_pFwdSolution; /**< The Mne Forward Solution. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;              /**< The Fiff Info. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RealTimeFwdSolution::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

//=========================================================================================================

inline bool RealTimeFwdSolution::isClustered()
{
    QMutexLocker locker(&m_qMutex);
    return m_bClustered;
}

} // NAMESPACE

#endif // REALTIMEFWDRESULT_H
