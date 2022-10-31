//=============================================================================================================
/**
 * @file     realtimefwdsolution.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
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
 * @brief    Contains the declaration of the RealTimeFwdSolution class.
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

namespace MNELIB{
    class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
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
    typedef QSharedPointer<RealTimeFwdSolution> SPtr;               /**< Shared pointer type for RealTimeFwdSolution. */
    typedef QSharedPointer<const RealTimeFwdSolution> ConstSPtr;    /**< Const shared pointer type for RealTimeFwdSolution. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeFwdSolution.
     */
    explicit RealTimeFwdSolution(QObject *parent = 0);

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
    void setFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Get the fiff info
     *
     * @return     The current fiff info.
     */
    QSharedPointer<FIFFLIB::FiffInfo> getFiffInfo();

    //=========================================================================================================
    /**
     * New forward solution to distribute
     *
     * @param[in] fwdSolution     The forward solution which should be distributed.
     */
    virtual void setValue(const MNELIB::MNEForwardSolution::SPtr pFwdSolution);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return The last attached value.
     */
    virtual QSharedPointer<MNELIB::MNEForwardSolution> getValue();

    //=========================================================================================================
    /**
     * New Lead Field to distribute
     *
     * @param[in] pNamedMatSol     The solution matrix (Lead Field) which should be distributed.
     */
    virtual void setSol(const FIFFLIB::FiffNamedMatrix::SDPtr& pNamedMatSol);

    //=========================================================================================================
    /**
     * Get the Lied Field matrix
     *
     * @return The Lead Field matrix.
     */
    virtual QSharedDataPointer<FIFFLIB::FiffNamedMatrix>& getSol();

    //=========================================================================================================
    /**
     * New gradient lead field to ditstribute
     *
     * @param[in] pNamedMatSolGrad     The gradient solution matrix (Lead Field) which should be distributed.
     */
    virtual void setSolGrad(const FIFFLIB::FiffNamedMatrix::SDPtr& pNamedMatSolGrad);

    //=========================================================================================================
    /**
     * Get the gradient Lied Field matrix
     *
     * @return The gradient lead field.
     */
    virtual QSharedDataPointer<FIFFLIB::FiffNamedMatrix>& getSolGrad();

    //=========================================================================================================
    /**
     * Returns if the fwd solution is clustered.
     *
     * @return if Forward is Clustered.
     */
    inline bool  isClustered();

    //=========================================================================================================
    /**
     * Returns whether RealTimeFwdSolution contains values
     *
     * @return whether RealTimeFwdSolution contains values.
     */
    inline bool isInitialized() const;

private:
    mutable QMutex          m_qMutex;                                       /**< Mutex to ensure thread safety. */
    bool                    m_bInitialized;                                 /**< If values are stored.*/
    bool                    m_bClustered;                                   /**< If fwd is clustered.*/

    QSharedPointer<MNELIB::MNEForwardSolution>      m_pFwdSolution;         /**< The Mne Forward Solution. */
    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;            /**< The Fiff Info. */

    QSharedDataPointer<FIFFLIB::FiffNamedMatrix>    m_pNamedMatSol;         /**< The solution matrix (LF). */
    QSharedDataPointer<FIFFLIB::FiffNamedMatrix>    m_pNamedMatSolGrad;     /**< The gradient solution matrix (LF). */
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

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeFwdSolution::SPtr)

#endif // REALTIMEFWDRESULT_H
