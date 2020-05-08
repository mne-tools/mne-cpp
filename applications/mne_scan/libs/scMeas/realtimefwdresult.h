//=============================================================================================================
/**
 * @file     realtimefwdresult.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeFwdResult class.
 *
 */

#ifndef REALTIMEFWDRESULT_H
#define REALTIMEFWDRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"

#include <mne/mne_forwardsolution.h>

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

namespace FIFFLIB{
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeFwdResult
 *
 * @brief The RealTimeFwdResult class provides a container for real-time HPI fitting results.
 */
class SCMEASSHARED_EXPORT RealTimeFwdResult : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeFwdResult> SPtr;               /**< Shared pointer type for RealTimeFwdResult. */
    typedef QSharedPointer<const RealTimeFwdResult> ConstSPtr;    /**< Const shared pointer type for RealTimeFwdResult. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeFwdResult.
     */
    explicit RealTimeFwdResult(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeFwdResult.
     */
    virtual ~RealTimeFwdResult();

    //=========================================================================================================
    /**
     * Set the fiff info
     *
     * @param [in] pFiffInfo     the new fiff info.
     */
    void setFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Get the fiff info
     *
     * @return     the current fiff info.
     */
    QSharedPointer<FIFFLIB::FiffInfo> getFiffInfo();

    //=========================================================================================================
    /**
     * New covariance to distribute
     *
     * @param [in] v     the covariance which should be distributed.
     */
    virtual void setValue(const MNELIB::MNEForwardSolution& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QSharedPointer<MNELIB::MNEForwardSolution>& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeFwdResult contains values
     *
     * @return whether RealTimeFwdResult contains values.
     */
    inline bool isInitialized() const;

private:
    mutable QMutex          m_qMutex;                                       /**< Mutex to ensure thread safety */
    bool                    m_bInitialized;                                 /**< If values are stored.*/

    QSharedPointer<MNELIB::MNEForwardSolution>      m_pMneFwdSol;        /**< The HPI fit result. */
    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;            /**< The Fiff Info. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RealTimeFwdResult::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeFwdResult::SPtr)

#endif // REALTIMEFWDRESULT_H
