//=============================================================================================================
/**
* @file     rtave.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
#include "Utils/detecttrigger.h"
#include "Utils/mnemath.h"


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
#include <QVariant>


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
    * @param[in] numAverages            Number of evkos to average
    * @param[in] p_iPreStimSamples      Number of samples averaged before the stimulus
    * @param[in] p_iPostStimSamples     Number of samples averaged after the stimulus (including the stimulus)
    * @param[in] p_iBaselineFromSecs    Start of baseline area which was/is used for correction in msecs
    * @param[in] p_iBaselineToSSecs     End of baseline area which was/is used for correction in msecs
    * @param[in] p_iTriggerIndex        Row in dex of channel which is to be scanned for triggers
    * @param[in] p_pFiffInfo            Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtAve(quint32 numAverages, quint32 p_iPreStimSamples, quint32 p_iPostStimSamples, quint32 p_iBaselineFromSecs, quint32 p_iBaselineToSecs, quint32 p_iTriggerIndex, FiffInfo::SPtr p_pFiffInfo, QObject *parent = 0);

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
    * Sets the number of averages
    *
    * @param[in] numAve     new number of averages
    */
    void setAverages(qint32 numAve);

    //=========================================================================================================
    /**
    * Sets the number of pre stimulus samples
    *
    * @param[in] samples    new number of pre stimulus samples
    * @param[in] secs    new number of pre stimulus seconds
    */
    void setPreStim(qint32 samples, qint32 secs);

    //=========================================================================================================
    /**
    * Sets the number of post stimulus samples
    *
    * @param[in] samples    new number of post stimulus samples
    * @param[in] secs    new number of pre stimulus seconds
    */
    void setPostStim(qint32 samples, qint32 secs);

    //=========================================================================================================
    /**
    * Sets the index of the trigger channel which is to be scanned fo triggers
    *
    * @param[in] idx    trigger channel index
    */
    void setTriggerChIndx(qint32 idx);

    //=========================================================================================================
    /**
    * Sets the baseline correction on or off
    *
    * @param[in] activate    activate baseline correction
    */
    void setBaselineActive(bool activate);

    //=========================================================================================================
    /**
    * Sets the from mSeconds of the baseline area
    *
    * @param[in] fromSamp    from of baseline area in samples
    * @param[in] fromMSec    from of baseline area in mSeconds
    */
    void setBaselineFrom(int fromSamp, int fromMSec);

    //=========================================================================================================
    /**
    * Sets the to mSeconds of the baseline area
    *
    * @param[in] toSamp    to of baseline area in samples
    * @param[in] toMSec    to of baseline area in mSeconds
    */
    void setBaselineTo(int toSamp, int toMSec);

    //=========================================================================================================
    /**
    * Sets the running average active
    *
    * @param[in] activate    activate running average
    */
    void setRunningAverageActive(bool activate);

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
    * Signal which is emitted when new evoked stimulus data are available.
    *
    * @param[out] p_pEvokedStim     The evoked stimulus data
    */
    void evokedStim(FIFFLIB::FiffEvoked::SPtr p_pEvokedStim);

    //=========================================================================================================
    /**
    * Emitted when number of averages changed
    */
    void numAveragesChanged();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    void clearDetectedTriggers();               /**< Clears already detected trigger*/

    void fillFrontBuffer(MatrixXd &data);       /**< Prepends incoming data to front/pre stim buffer*/
    int fillBackBuffer(MatrixXd &data);         /**< Prepends incoming data to back/post stim buffer*/
    void mergeData();                           /**< Packs the buffers togehter as one and calcualtes the current running average and emits the result if number of averages has been reached*/
    void generateEvoked();                      /**< Generates the final evoke variable*/
    void reset();
    void init();

    QMutex  m_qMutex;                   /**< Provides access serialization between threads*/

    qint32  m_iNumAverages;             /**< Number of averages */
    qint32  m_iCurrentBlockSize;        /**< Current block size of the incoming data */
    qint32  m_iPreStimSamples;          /**< Amount of samples averaged before the stimulus. */
    qint32  m_iPostStimSamples;         /**< Amount of samples averaged after the stimulus, including the stimulus sample.*/
    qint32  m_iNewPreStimSamples;       /**< New amount of samples averaged before the stimulus. */
    qint32  m_iNewPostStimSamples;      /**< New amount of samples averaged after the stimulus, including the stimulus sample.*/
    qint32  m_iPreStimSeconds;          /**< Amount of seconds averaged before the stimulus. */
    qint32  m_iPostStimSeconds;         /**< Amount of seconds averaged after the stimulus, including the stimulus sample.*/
    qint32  m_iCurrentMatBufferIndex;   /**< Current index inside of the matrix buffer m_matBuffer */
    qint32  m_iTriggerIndex;            /**< Current row index of the data matrix which is to be scanned for triggers */
    qint32  m_iNewTriggerIndex;         /**< Old row index of the data matrix which is to be scanned for triggers */
    qint32  m_iTriggerPos;              /**< Last found trigger postion */

    float   m_fTriggerThreshold;        /**< Threshold to detect trigger */

    bool    m_bIsRunning;               /**< Holds if real-time Covariance estimation is running.*/
    bool    m_bAutoAspect;              /**< Auto aspect detection on or off. */
    bool    m_bFillingBackBuffer;       /**< Whether the back buffer is currently getting filled. */
    bool    m_bRunningAverage;          /**< Whether the running average is to be calculated. */
    bool    m_bDoBaselineCorrection;    /**< Whether to perform baseline correction. */
    bool    m_bNewRunningAverage;       /**< Whether to perform running or cumulative averaging. */

    QPair<QVariant,QVariant>    m_pairBaselineSec;     /**< Baseline information in seconds form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/
    QPair<QVariant,QVariant>    m_pairBaselineSamp;     /**< Baseline information in samples form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/

    FiffInfo::SPtr          m_pFiffInfo;            /**< Holds the fiff measurement information. */
    FiffEvoked::SPtr        m_pStimEvoked;          /**< Holds the evoked information. */

    CircularMatrixBuffer<double>::SPtr m_pRawMatrixBuffer;      /**< The Circular Raw Matrix Buffer. */

    QMap<int,QList<int> >   m_qMapDetectedTrigger;              /**< Detected trigger for each trigger channel. */

    MatrixXd                m_matStimData;                      /**< The matrix data correspdoning to the latest detected stim event. */
    QList<MatrixXd>         m_matBufferFront;                   /**< the front/pre stim data buffer for each trigger channel. This buffer and its including matrices should always sum up to m_iPreStimSamples columns */
    QList<MatrixXd>         m_matBufferBack;                    /**< the back/post stim data buffer for each trigger channel. This buffer and its including matrices should always sum up to m_iPostStimSamples columns */
    QList<MatrixXd>         m_qListStimAve;                     /**< the current stimulus average buffer. Holds m_iNumAverages vectors */
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
