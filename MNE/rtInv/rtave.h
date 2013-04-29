//=============================================================================================================
/**
* @file     rtave.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     RtAve class declaration.
*
*/

#ifndef RTAVE_H
#define RTAVE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinv_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked.h>
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
#include <QSet>
#include <QList>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVRTLIB
//=============================================================================================================

namespace RTINVLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace IOBuffer;
using namespace FIFFLIB;


//=============================================================================================================
/**
* Real-time averaging and returns evoked data
*
* @brief Real-time averaging helper
*/
class RTINVSHARED_EXPORT RtAve : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtAve> SPtr;             /**< Shared pointer type for RtCov. */
    typedef QSharedPointer<const RtAve> ConstSPtr;  /**< Const shared pointer type for RtCov. */

    //=========================================================================================================
    /**
    * Creates the real-time covariance estimation object.
    *
    * @param[in] p_iMaxSamples      Number of samples to use for each data chunk
    * @param[in] p_pFiffInfo        Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtAve(qint32 p_iMaxSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Real-time covariance estimation object.
    */
    ~RtAve();

    //=========================================================================================================
    /**
    * Slot to receive incoming data.
    *
    * @param[in] p_DataSegment  Data to estimate the covariance from -> ToDo Replace this by shared data pointer
    */
    void append(const MatrixXd &p_DataSegment);

    //=========================================================================================================
    /**
    * Stops the RtCov by stopping the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns true if is running, otherwise false.
    *
    * @return true if is running, false otherwise
    */
    inline bool isRunning();

signals:
    //=========================================================================================================
    /**
    * Signal which is emitted when new evoked data are available.
    *
    * @param[out] p_pEvoked     The evoked data
    */
    void evokedCalculated(FIFFLIB::FiffEvoked::SPtr p_pEvoked);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    FiffInfo::SPtr  m_pFiffInfo;        /**< Holds the fiff measurement information. */

    QMutex      mutex;                  /**< Provides access serialization between threads*/
    bool        m_bIsRunning;           /**< Holds if real-time Covariance estimation is running.*/

    quint32      m_iMaxSamples;         /**< Maximal amount of samples received, before covariance is estimated.*/

    CircularMatrixBuffer<double>::SPtr m_pRawMatrixBuffer;   /**< The Circular Raw Matrix Buffer. */

    bool m_bAutoAspect; /**< Auto aspect detection on or off. */


    QList<qint32> m_qListStimChannelIdcs;      /**< Stimulus channel indeces. */

    QSet<fiff_int_t>  m_qSetAspectKinds;     /**< Set of aspects to average. Each aspect is averaged separetely and released stored in evoked data.*/

    QList< QVector<MatrixXd> > m_qListQVectorPreStimuli;    /**< averages the pre stimuli */
    QList< QVector<MatrixXd> > m_qListQVectorPostStimuli;    /**< averages the post stimuli */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtAve::isRunning()
{
    return m_bIsRunning;
}

} // NAMESPACE

#ifndef metatype_fiffevokedsptr
#define metatype_fiffevokedsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffEvoked::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffEvoked type. For signal/slot usage.*/
#endif

#endif // RTAVE_H
