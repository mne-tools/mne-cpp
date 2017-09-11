//=============================================================================================================
/**
* @file     cshdataworker.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     CshDataWorker class declaration.
*
*/

#ifndef CSH_CSHDATAWORKER_H
#define CSH_CSHDATAWORKER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computeShader_global.h"
#include <list>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QThread>
#include <QMutex>


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
// DEFINE NAMESPACE CSH
//=============================================================================================================

namespace CSH {


//*************************************************************************************************************
//=============================================================================================================
// CSH FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Worker which schedules data with the right timing.
*
* @brief Data scheduler
*/

class COMPUTE_SHADERSHARED_EXPORT CshDataWorker : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<CshDataWorker> SPtr;            /**< Shared pointer type for CshDataWorker. */
    typedef QSharedPointer<const CshDataWorker> ConstSPtr; /**< Const shared pointer type for CshDataWorker. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit CshDataWorker(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~CshDataWorker();

    //=========================================================================================================
    /**
    * Add data which is to be streamed.
    *
    * @param[in] tData         The new data.
    */
    void addData(const Eigen::MatrixXf& tData);

    //=========================================================================================================
    /**
    * Clear this worker, empties the m_lData field that holds the current block of sensor activity
    */
    void clear();

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] tNumAvr                The new number of averages.
    */
    void setNumberAverages(const uint tNumAvr);

    //=========================================================================================================
    /**
    * Set the length in milli Seconds to wait inbetween data samples.
    *
    * @param[in] tMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setInterval(const uint tMSec);

    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] tLooping                The new looping state.
    */
    void setLoop(const bool tLooping);

    //=========================================================================================================
    /**
    * Set the sampling frequency.
    *
    * @param[in] tSFreq                 The new sampling frequency.
    */
    void setSFreq(const double tSFreq);

    //=========================================================================================================
    /**
    * Sets the running flag to false and waits for the worker to stop.
    */
    void stop();

    //=========================================================================================================
    /**
    * Resets the index of the current sample and starts the worker.
    */
    void start();

protected:

    //=========================================================================================================
    /**
    * Main method of this worker: Checks whether it is time for the worker to output new data for visualization.
    * If so, it averages the specified amount of data samples and calculates the output.
    */
    virtual void run() override;

private:

    //=========================================================================================================
    QMutex                                      m_qMutex;                           /**< The thread's mutex. */

    std::list<Eigen::VectorXf>                  m_lData;                            /**< List that holds the fiff matrix data <n_channels x n_samples>. */
    std::list<Eigen::VectorXf>::const_iterator  m_itCurrentSample;                  /**< Iterator to the current Sample in the linked list. */

    bool                                        m_bIsRunning;                       /**< Flag if this thread is running. */
    bool                                        m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */

    uint                                        m_iAverageSamples;                  /**< Number of average to compute. */
    uint                                        m_iMSecIntervall;                   /**< Length in milli Seconds to wait inbetween data samples. */

    double                                      m_dSFreq;                           /**< The current sampling frequency. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send new signal data to its listeners.
    *
    * @param[in] tDataMatrix     The signal data from each sensor.
    */
    void newRtData(const Eigen::VectorXf &tDataMatrix);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#ifndef metatype_vectorxd
#define metatype_vectorxd
Q_DECLARE_METATYPE(Eigen::VectorXf);
#endif

#endif // CSH_CSHDATAWORKER_H
