//=============================================================================================================
/**
 * @file     rtnoise.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     RtNoise class declaration.
 *
 */

#ifndef RTNOISE_RTPROCESSING_H
#define RTNOISE_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * Real-time noise Spectrum estimation
 *
 * @brief Real-time Noise estimation
 */
class RTPROCESINGSHARED_EXPORT RtNoise : public QThread
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtNoise> SPtr;             /**< Shared pointer type for RtNoise. */
    typedef QSharedPointer<const RtNoise> ConstSPtr;  /**< Const shared pointer type for RtNoise. */

    //=========================================================================================================
    /**
     * Creates the real-time covariance estimation object.
     *
     * @param[in] p_iMaxSamples      Number of samples to use for each data chunk.
     * @param[in] p_pFiffInfo        Associated Fiff Information.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtNoise(qint32 p_iMaxSamples,
                     FIFFLIB::FiffInfo::SPtr p_pFiffInfo,
                     qint32 p_dataLen,
                     QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the Real-time noise estimation object.
     */
    ~RtNoise();

    //=========================================================================================================
    /**
     * Slot to receive incoming data.
     *
     * @param[in] p_DataSegment  Data to estimate the spectrum from -> ToDo Replace this by shared data pointer.
     */
    void append(const Eigen::MatrixXd &p_DataSegment);

    //=========================================================================================================
    /**
     * Returns true if is running, otherwise false.
     *
     * @return true if is running, false otherwise.
     */
    inline bool isRunning();

    //=========================================================================================================
    /**
     * Starts the RtNoise by starting the producer's thread.
     *
     * @return true if succeeded, false otherwise.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the RtNoise by stopping the producer's thread.
     *
     * @return true if succeeded, false otherwise.
     */
    virtual bool stop();

    QMutex ReadMutex;

    Eigen::MatrixXd m_matSpecData;
    bool m_bSendDataToBuffer;

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

    QVector <float> hanning(int N, short itype);

    int m_iNumOfBlocks;
    int m_iBlockSize;
    int m_iSensors;
    int m_iBlockIndex;

    Eigen::MatrixXd m_matCircBuf;

private:
    QMutex      mutex;                              /**< Provides access serialization between threads*/

    FIFFLIB::FiffInfo::SPtr  m_pFiffInfo;           /**< Holds the fiff measurement information. */

    bool        m_bIsRunning;                       /**< Holds if real-time Covariance estimation is running.*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>       m_pCircularBuffer;      /**< Holds incoming raw data. */

    QVector <float> m_fWin;

    double m_Fs;

    qint32 m_iFftLength;
    qint32 m_dataLength;

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when a new data Matrix is estimated.
     *
     * @param[out].
     */
    void SpecCalculated(Eigen::MatrixXd);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtNoise::isRunning()
{
    return m_bIsRunning;
}
} // NAMESPACE

#ifndef metatype_matrix
#define metatype_matrix
Q_DECLARE_METATYPE(Eigen::MatrixXd); /**< Provides QT META type declaration of the Eigen::MatrixXd type. For signal/slot usage.*/
#endif

#endif // RTNOISE_RTPROCESSING_H
