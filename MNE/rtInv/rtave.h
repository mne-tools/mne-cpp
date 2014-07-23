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
    * @param[in] numAverages        Number of evkos to average
    * @param[in] p_iPreStimSamples  Number of samples averaged before the stimulus
    * @param[in] p_iPostStimSamples Number of samples averaged after the stimulus (including the stimulus)
    * @param[in] p_pFiffInfo        Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtAve(quint32 numAverages, quint32 p_iPreStimSamples, quint32 p_iPostStimSamples, FiffInfo::SPtr p_pFiffInfo, QObject *parent = 0);

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
    * Starts the RtAve by starting the producer's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the RtAve by stopping the producer's thread.
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
    * Signal which is emitted when new evoked pre stimulus data are available.
    *
    * @param[out] p_pEvokedPreStim     The evoked pre stimulus data
    */
    void evokedPreStim(FIFFLIB::FiffEvoked::SPtr p_pEvokedPreStim);

    //=========================================================================================================
    /**
    * Signal which is emitted when new evoked post stimulus data are available.
    *
    * @param[out] p_pEvokedPostStim     The evoked post stimulus data
    */
    void evokedPostStim(FIFFLIB::FiffEvoked::SPtr p_pEvokedPostStim);

    //=========================================================================================================
    /**
    * Signal which is emitted when new evoked stimulus data are available.
    *
    * @param[out] p_pEvokedStim     The evoked stimulus data
    */
    void evokedStim(FIFFLIB::FiffEvoked::SPtr p_pEvokedStim);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Assemble the poststimulus
    *
    * @param[in] p_qListRawMatBuf   List of raw buffers
    * @param[in] p_iStimIdx         Stimulus index to investigate
    */
    void assemblePostStimulus(const QList<QPair<QList<qint32>, MatrixXd> > &p_qListRawMatBuf, qint32 p_iStimIdx);

    //=========================================================================================================
    /**
    * Assemble the prestimulus
    *
    * @param[in] p_qListRawMatBuf   List of raw buffers
    * @param[in] p_iStimIdx         Stimulus index to investigate
    */
    void assemblePreStimulus(const QList<QPair<QList<qint32>, MatrixXd> > &p_qListRawMatBuf, qint32 p_iStimIdx);

    QMutex      mutex;                  /**< Provides access serialization between threads*/

    qint32 m_iNumAverages;              /**< Number of averages */

    qint32     m_iPreStimSamples;       /**< Amount of samples averaged before the stimulus. */
    qint32     m_iPostStimSamples;      /**< Amount of samples averaged after the stimulus, including the stimulus sample.*/

    FiffInfo::SPtr  m_pFiffInfo;        /**< Holds the fiff measurement information. */

    bool        m_bIsRunning;           /**< Holds if real-time Covariance estimation is running.*/

    CircularMatrixBuffer<double>::SPtr m_pRawMatrixBuffer;   /**< The Circular Raw Matrix Buffer. */

    bool m_bAutoAspect; /**< Auto aspect detection on or off. */


    QList<qint32> m_qListStimChannelIdcs;   /**< Stimulus channel indeces. */

//    QList<fiff_int_t>  m_qSetAspectKinds;   /**< List of aspects to average. Each aspect is averaged separetely and released stored in evoked data.*/

    QList<QList<MatrixXd> > m_qListQListPreStimBuf;     /**< assembles the pre stimulus data */
    QList<QList<MatrixXd> > m_qListQListPostStimBuf;    /**< assembles the post stimulus data */

    QList<MatrixXd> m_qListPreStimAve;     /**< the current pre stimulus average */
    QList<MatrixXd> m_qListPostStimAve;    /**< the current post stimulus average */
    QList<MatrixXd> m_qListStimAve;     /**< the current stimulus average */
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
