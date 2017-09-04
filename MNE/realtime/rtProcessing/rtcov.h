//=============================================================================================================
/**
* @file     rtcov.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     RtCov class declaration.
*
*/

#ifndef RTCOV_H
#define RTCOV_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../realtime_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Generics INCLUDES
//=============================================================================================================

#include <generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REALTIMELIB
//=============================================================================================================

namespace REALTIMELIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace IOBUFFER;
using namespace FIFFLIB;


//=============================================================================================================
/**
* Real-time covariance estimation
*
* @brief Real-time covariance estimation
*/
class REALTIMESHARED_EXPORT RtCov : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtCov> SPtr;             /**< Shared pointer type for RtCov. */
    typedef QSharedPointer<const RtCov> ConstSPtr;  /**< Const shared pointer type for RtCov. */

    //=========================================================================================================
    /**
    * Creates the real-time covariance estimation object.
    *
    * @param[in] p_iMaxSamples      Number of samples to use for each data chunk
    * @param[in] p_pFiffInfo        Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtCov(qint32 p_iMaxSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Real-time covariance estimation object.
    */
    ~RtCov();

    //=========================================================================================================
    /**
    * Slot to receive incoming data.
    *
    * @param[in] p_DataSegment  Data to estimate the covariance from -> ToDo Replace this by shared data pointer
    */
    void append(const MatrixXd &p_DataSegment);

    //=========================================================================================================
    /**
    * Returns true if is running, otherwise false.
    *
    * @return true if is running, false otherwise
    */
    inline bool isRunning();

    //=========================================================================================================
    /**
    * Set number of estimation samples
    *
    * @param[in] samples    estimation samples to set
    */
    void setSamples(qint32 samples);

    //=========================================================================================================
    /**
    * Starts the RtCov by starting the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the RtCov by stopping the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool stop();

signals:
    //=========================================================================================================
    /**
    * Signal which is emitted when a new covariance Matrix is estimated.
    *
    * @param[out] p_pCov  The covariance matrix
    */
    void covCalculated(FIFFLIB::FiffCov::SPtr p_pCov);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    QMutex      mutex;                  /**< Provides access serialization between threads*/

    quint32      m_iMaxSamples;         /**< Maximal amount of samples received, before covariance is estimated.*/

    quint32      m_iNewMaxSamples;      /**< New maximal amount of samples received, before covariance is estimated.*/

    FiffInfo::SPtr  m_pFiffInfo;        /**< Holds the fiff measurement information. */

    bool        m_bIsRunning;           /**< Holds if real-time Covariance estimation is running.*/

    CircularMatrixBuffer<double>::SPtr m_pRawMatrixBuffer;   /**< The Circular Raw Matrix Buffer. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtCov::isRunning()
{
    return m_bIsRunning;
}

} // NAMESPACE

#ifndef metatype_fiffcovsptr
#define metatype_fiffcovsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffCov::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffCov type. For signal/slot usage.*/
#endif

#endif // RTCOV_H
