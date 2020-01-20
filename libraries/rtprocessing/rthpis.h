//=============================================================================================================
/**
 * @file     rthpis.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief     RtHPIS class declaration.
 *
 */

#ifndef RTHPIS_H
#define RTHPIS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <utils/generics/circularmatrixbuffer.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_coord_trans.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{


//*************************************************************************************************************
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================
/**
 * The struct specifing all data needed to perform coil-wise fitting.
 */
struct FittingResult {
    FIFFLIB::FiffDigPointSet fittedCoils;
    FIFFLIB::FiffCoordTrans devHeadTrans;
    QVector<double> errorDistances;
};

//=============================================================================================================
/**
 * Real-time HPI worker.
 *
 * @brief Real-time HPI worker.
 */
class RTPROCESINGSHARED_EXPORT RtHPISWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Perform one single HPI fit.
    *
    * @param[in] matData            Data to estimate the HPI positions from
    * @param[in] matProjectors      The projectors to apply. Bad channels are still included.
    * @param[in] vFreqs             The frequencies for each coil.
    * @param[in] pFiffInfo          Associated Fiff Information.
    */
    void doWork(const Eigen::MatrixXd& matData,
                const Eigen::MatrixXd& matProjectors,
                const QVector<int>& vFreqs,
                QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

signals:
    void resultReady(const RTPROCESSINGLIB::FittingResult &fitResult);
};

//=============================================================================================================
/**
 * Real-time Head Coil Positions estimation.
 *
 * @brief Real-time Head Coil Positions estimation.
 */
class RTPROCESINGSHARED_EXPORT RtHPIS : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtHPIS> SPtr;             /**< Shared pointer type for RtHPIS. */
    typedef QSharedPointer<const RtHPIS> ConstSPtr;  /**< Const shared pointer type for RtHPIS. */

    //=========================================================================================================
    /**
    * Creates the real-time HPIS estimation object.
    *
    * @param[in] p_pFiffInfo        Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtHPIS(QSharedPointer<FIFFLIB::FiffInfo> p_pFiffInfo,
                    QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Real-time HPI estimation object.
    */
    ~RtHPIS();    

    //=========================================================================================================
    /**
    * Slot to receive incoming data.
    *
    * @param[in] data  Data to estimate the HPI positions from
    */
    void append(const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
    * Set the coil frequencies.
    *
    * @param[in] vCoilFreqs  The coil frequencies.
    */
    void setCoilFrequencies(const QVector<int>& vCoilFreqs);

    //=========================================================================================================
    /**
    * Set the new projection matrix.
    *
    * @param[in] matProjectors  The new projection matrix.
    */
    void setProjectionMatrix(const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
    * Restarts the thread by interrupting its computation queue, quitting, waiting and then starting it again.
    */
    void restart();

    //=========================================================================================================
    /**
    * Stops the thread by interrupting its computation queue, quitting and waiting.
    */
    void stop();

protected:
    //=========================================================================================================
    /**
    * Handles the results.
    */
    void handleResults(const FittingResult &fitResult);

    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;           /**< Holds the fiff measurement information. */

    QThread             m_workerThread;         /**< The worker thread. */
    QVector<int>        m_vCoilFreqs;           /**< Vector contains the HPI coil frequencies. */
    Eigen::MatrixXd     m_matProjectors;        /**< Holds the matrix with the SSP and compensator projectors.*/

signals:
    void newFittingResultAvailable(const RTPROCESSINGLIB::FittingResult &fitResult);
    void operate(const Eigen::MatrixXd& matData,
                 const Eigen::MatrixXd& matProjectors,
                 const QVector<int>& vFreqs,
                 QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#ifndef metatype_rthpisfittingresult
#define metatype_rthpisfittingresult
Q_DECLARE_METATYPE(RTPROCESSINGLIB::FittingResult)
#endif

#endif // RTHPIS_H
