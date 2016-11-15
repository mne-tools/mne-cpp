//=============================================================================================================
/**
* @file     rthpis.h
* @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@ntu-ilmenau.de>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Chiran Doshi, Lorenz Esch, Limin Sun, and Matti Hamalainen. All rights reserved.
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
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace IOBUFFER;
using namespace FIFFLIB;

//*************************************************************************************************************
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

struct coilParam {
    Eigen::MatrixXd pos;
    Eigen::MatrixXd mom;
    Eigen::VectorXd dpfiterror;
    Eigen::VectorXd dpfitnumitr;
};

struct dipError {
    double error;
    Eigen::MatrixXd moment;
    int numIterations;
};

struct sens {
    Eigen::MatrixXd coilpos;
    Eigen::MatrixXd coilori;
    Eigen::MatrixXd tra;
};


//=============================================================================================================
/**
* Real-time Head Coil Positions estimation
*
* @brief Real-time HPI estimation
*/
class RTPROCESSINGSHARED_EXPORT RtHPIS : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtHPIS> SPtr;             /**< Shared pointer type for RtHPIS. */
    typedef QSharedPointer<const RtHPIS> ConstSPtr;  /**< Const shared pointer type for RtHPIS. */

    //=========================================================================================================
    /**
    * Creates the real-time HPIS estimation object.
    *
    * @param[in] p_iMaxSamples      Number of samples to use for each data chunk
    * @param[in] p_pFiffInfo        Associated Fiff Information
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtHPIS(FiffInfo::SPtr p_pFiffInfo, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Real-time HPI estimation object.
    */
    ~RtHPIS();

    //=========================================================================================================
    /**
    * Inits the rt HPI processing and performs one single fit.
    *
    * @param[in] t_mat      Data to estimate the HPI positions from
    * @param[in] vGof       The goodness of fit in mm for each fitted HPI coil.
    * @param[in] vFreqs     The frequencies for each coil.
    */
    void singleHPIFit(const MatrixXd& t_mat, const QVector<int>& vFreqs, QVector<double> &vGof);

    //=========================================================================================================
    /**
    * Slot to receive incoming data.
    *
    * @param[in] p_DataSegment  Data to estimate the HPI positions from
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
    * Starts the RtHPIS by starting the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the RtHPIS by stopping the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool stop();

    coilParam dipfit(struct coilParam, struct sens, Eigen::MatrixXd, int numCoils);
    Eigen::Matrix4d computeTransformation(Eigen::MatrixXd, Eigen::MatrixXd);
    Eigen::MatrixXd fminsearch(Eigen::MatrixXd,int, int, int, Eigen::MatrixXd, struct sens);

    //void test();

    QMutex m_mutex;

    bool SendDataToBuffer;

    int simplex_numitr;

    //MatrixXd SpecData;

signals:
    //=========================================================================================================
    /**
    * Signal which is emitted when a new data Matrix is estimated.
    *
    * @param[out]
    */
    void HPICalculated(Eigen::MatrixXd);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

    CircularMatrixBuffer<double>::SPtr m_pRawMatrixBuffer;      /**< The Circular Raw Matrix Buffer. */

    QMutex              mutex;                                  /**< Provides access serialization between threads*/

    quint32             m_iMaxSamples;                          /**< Maximal amount of samples received, before covariance is estimated.*/
    quint32             m_iNewMaxSamples;                       /**< New maximal amount of samples received, before covariance is estimated.*/

    FiffInfo::SPtr      m_pFiffInfo;                            /**< Holds the fiff measurement information. */

    bool                m_bIsRunning;                           /**< Holds if real-time Covariance estimation is running.*/

    static std::vector <double>base_arr;

    int                 m_iCounter;
    //QVector <float> m_fWin;

    //double m_Fs;

    //qint32 m_iFFTlength;
    //qint32 m_dataLength;

    //int NumOfBlocks;
    //int BlockSize  ;
    //int Sensors    ;
    //int BlockIndex ;

    //MatrixXd CircBuf;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtHPIS::isRunning()
{
    return m_bIsRunning;
}

} // NAMESPACE

#ifndef metatype_matrix
#define metatype_matrix
Q_DECLARE_METATYPE(Eigen::MatrixXd); /**< Provides QT META type declaration of the MatrixXd type. For signal/slot usage.*/
#endif

#endif // RTHPIS_H
