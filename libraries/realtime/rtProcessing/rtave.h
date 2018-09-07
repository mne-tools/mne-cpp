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

#include "../realtime_global.h"

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_info.h>

#include <utils/generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
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
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REALTIMELIB
//=============================================================================================================

namespace REALTIMELIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Real-time averaging and returns evoked data
*
* @brief Real-time averaging helper
*/
class REALTIMESHARED_EXPORT RtAve : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtAve> SPtr;             /**< Shared pointer type for RtAve. */
    typedef QSharedPointer<const RtAve> ConstSPtr;  /**< Const shared pointer type for RtAve. */

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
    explicit RtAve(quint32 numAverages,
                   quint32 p_iPreStimSamples,
                   quint32 p_iPostStimSamples,
                   quint32 p_iBaselineFromSecs,
                   quint32 p_iBaselineToSecs,
                   quint32 p_iTriggerIndex,
                   FIFFLIB::FiffInfo::SPtr p_pFiffInfo,
                   QObject *parent = 0);

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
    void append(const Eigen::MatrixXd &p_DataSegment);

    //=========================================================================================================
    /**
    * Sets the number of averages
    *
    * @param[in] numAve     new number of averages
    */
    void setAverages(qint32 numAve);

    //=========================================================================================================
    /**
    * Sets the average mode
    *
    * @param[in] mode     average mode (0-running or 1-cumulative)
    */
    void setAverageMode(qint32 mode);

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
    * Sets the artifact reduction
    *
    * @param[in] bActivateThreshold     Whether to activate threshold artifact reduction or not
    * @param[in] dValueThreshold        The artifact threshold value
    * @param[in] bActivateVariance      Whether to activate variance artifact reduction or not
    * @param[in] dValueVariance         The artifact variance value
    */
    void setArtifactReduction(bool bActivateThreshold, double dValueThreshold, bool bActivateVariance, double dValueVariance);

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

    //=========================================================================================================
    /**
    * Resets the averaged data stored.
    */
    void reset();

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
    * do the actual averaging here.
    */
    void doAveraging(const Eigen::MatrixXd& rawSegment);

    //=========================================================================================================
    /**
    * Prepends incoming data to front/pre stim buffer.
    */
    void fillFrontBuffer(const Eigen::MatrixXd& data, double dTriggerType);

    //=========================================================================================================
    /**
    * Prepends incoming data to back/post stim buffer.
    */
    void fillBackBuffer(const Eigen::MatrixXd& data, double dTriggerType);

    //=========================================================================================================
    /**
    * Packs the buffers togehter as one and calcualtes the current running average and emits the result if number of averages has been reached.
    */
    void mergeData(double dTriggerType);

    //=========================================================================================================
    /**
    * Generates the final evoke variable.
    */
    void generateEvoked(double dTriggerType);

    //=========================================================================================================
    /**
    * Checks the givven matrix for artifacts beyond a threshold value.
    *
    * @param[in] data           The data matrix.
    *
    * @return   Whether a thresold artifact was detected.
    */
    bool checkForArtifact(Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
    * Check if data buffer has been initialized
    */
    inline bool isDataBufferInit();

    //=========================================================================================================
    /**
    * Check if control values have been changed
    */
    inline bool controlValuesChanged();

    QMutex                                          m_qMutex;                   /**< Provides access serialization between threads*/

    qint32                                          m_iNumAverages;             /**< Number of averages */
    qint32                                          m_iNewNumAverages;          /**< Number of averages */

    qint32                                          m_iPreStimSamples;          /**< Amount of samples averaged before the stimulus. */
    qint32                                          m_iNewPreStimSamples;       /**< New amount of samples averaged before the stimulus. */

    qint32                                          m_iPostStimSamples;         /**< Amount of samples averaged after the stimulus, including the stimulus sample.*/
    qint32                                          m_iNewPostStimSamples;      /**< New amount of samples averaged after the stimulus, including the stimulus sample.*/

    qint32                                          m_iAverageMode;             /**< The averaging mode 0-running 1-cumulative. */
    qint32                                          m_iNewAverageMode;          /**< The new averaging mode 0-running 1-cumulative. */

    qint32                                          m_iTriggerChIndex;          /**< Current row index of the data matrix which is to be scanned for triggers */
    qint32                                          m_iNewTriggerIndex;         /**< Old row index of the data matrix which is to be scanned for triggers */

    float                                           m_fTriggerThreshold;        /**< Threshold to detect trigger */

    bool                                            m_bActivateThreshold;       /**< Whether to do threshold artifact reduction or not. */
    bool                                            m_bActivateVariance;        /**< Whether to do variance artifact reduction or not. */
    bool                                            m_bIsRunning;               /**< Holds if real-time Covariance estimation is running.*/
    bool                                            m_bAutoAspect;              /**< Auto aspect detection on or off. */
    bool                                            m_bDoBaselineCorrection;    /**< Whether to perform baseline correction. */

    QPair<QVariant,QVariant>                        m_pairBaselineSec;          /**< Baseline information in seconds form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/
    QPair<QVariant,QVariant>                        m_pairBaselineSamp;         /**< Baseline information in samples form where the seconds are seen relative to the trigger, meaning they can also be negative [from to]*/

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                /**< Holds the fiff measurement information. */
    FIFFLIB::FiffEvokedSet::SPtr                    m_pStimEvokedSet;           /**< Holds the evoked information. */

    QMap<int,QList<int> >                           m_qMapDetectedTrigger;      /**< Detected trigger for each trigger channel. */
    QMap<double,QList<Eigen::MatrixXd> >            m_mapStimAve;               /**< the current stimulus average buffer. Holds m_iNumAverages vectors */
    QMap<double,Eigen::MatrixXd>                    m_mapDataPre;               /**< The matrix holding the pre stim data. */
    QMap<double,Eigen::MatrixXd>                    m_mapDataPost;              /**< The matrix holding the post stim data. */
    QMap<double,qint32>                             m_mapMatDataPostIdx;        /**< Current index inside of the matrix m_matDataPost */
    QMap<double,bool>                               m_mapFillingBackBuffer;     /**< Whether the back buffer is currently getting filled. */
    QMap<double,qint32>                             m_mapNumberCalcAverages;    /**< The number of currently calculated averages for each trigger type. */

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pRawMatrixBuffer;         /**< The Circular Raw Matrix Buffer. */

    QStringList                                     m_lResponsibleTriggerTypes; /**< List of all trigger types which lead to the recent emit of a new evoked set. */

signals:
    //=========================================================================================================
    /**
    * Signal which is emitted when new evoked stimulus data are available.
    *
    * @param[in] p_pEvokedStimSet          The evoked stimulus data set.
    * @param[in] lResponsibleTriggerTypes  List of all trigger types which lead to the recent emit of a new evoked set.
    */
    void evokedStim(FIFFLIB::FiffEvokedSet::SPtr p_pEvokedStimSet, const QStringList& lResponsibleTriggerTypes);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtAve::isRunning()
{
    return m_bIsRunning;
}


//*************************************************************************************************************

inline bool RtAve::isDataBufferInit()
{
    QMutexLocker locker(&m_qMutex);

    bool result = false;

    if(m_pRawMatrixBuffer) {
        result = true;
    }

    return result;
}


//*************************************************************************************************************

inline bool RtAve::controlValuesChanged()
{
    QMutexLocker locker(&m_qMutex);

    bool result = false;

    if(m_iNewPreStimSamples != m_iPreStimSamples
            || m_iNewPostStimSamples != m_iPostStimSamples
            || m_iNewTriggerIndex != m_iTriggerChIndex
            || m_iNewAverageMode != m_iAverageMode
            || m_iNewNumAverages != m_iNumAverages) {
        result = true;
    }

    return result;
}

} // NAMESPACE

#ifndef metatype_fiffevokedsptr
#define metatype_fiffevokedsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffEvoked::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffEvoked type. For signal/slot usage.*/
#endif

#endif // RTAVE_H
