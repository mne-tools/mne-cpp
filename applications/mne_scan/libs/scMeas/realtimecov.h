//=============================================================================================================
/**
 * @file     realtimecov.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeCov class.
 *
 */

#ifndef REALTIMECOV_H
#define REALTIMECOV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
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
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
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
    typedef QSharedPointer<RealTimeCov> SPtr;               /**< Shared pointer type for RealTimeCov. */
    typedef QSharedPointer<const RealTimeCov> ConstSPtr;    /**< Const shared pointer type for RealTimeCov. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeCov.
     */
    explicit RealTimeCov(QObject *parent = 0);

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
     * @param[in] v     the covariance which should be distributed.
     */
    virtual void setValue(const FIFFLIB::FiffCov& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual FIFFLIB::FiffCov::SPtr& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeCov contains values
     *
     * @return whether RealTimeCov contains values.
     */
    inline bool isInitialized() const;

private:
    mutable QMutex          m_qMutex;       /**< Mutex to ensure thread safety. */

    FIFFLIB::FiffCov::SPtr  m_pFiffCov;     /**< Covariance data set. */
    FIFFLIB::FiffInfo::SPtr m_pFiffInfo;    /**< The Fiff Info. */

    bool                    m_bInitialized; /**< If values are stored.*/
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

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeCov::SPtr)

#endif // REALTIMECOV_H
