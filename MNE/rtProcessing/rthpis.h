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

#include <generics/circularmatrixbuffer.h>


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
    class FiffCoordTrans;
    class FiffDigPointSet;
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
//=========================================================================================================
/**
* The strucut specifing the coil parameters.
*/
struct coilParam {
    Eigen::MatrixXd pos;
    Eigen::MatrixXd mom;
    Eigen::VectorXd dpfiterror;
    Eigen::VectorXd dpfitnumitr;
};

//=========================================================================================================
/**
* The strucut specifing the dipole error.
*/
struct dipError {
    double error;
    Eigen::MatrixXd moment;
    int numIterations;
};

//=========================================================================================================
/**
* The strucut specifing the sensor parameters.
*/
struct sens {
    Eigen::MatrixXd coilpos;
    Eigen::MatrixXd coilori;
    Eigen::MatrixXd tra;
};

//=========================================================================================================
/**
* The strucut specifing the sorting parameters.
*/
struct sortStruct {
    double base_arr;
    int idx;
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
    explicit RtHPIS(QSharedPointer<FIFFLIB::FiffInfo> p_pFiffInfo, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Real-time HPI estimation object.
    */
    ~RtHPIS();

    //=========================================================================================================
    /**
    * Inits the rt HPI processing and performs one single fit.
    *
    * @param[in] t_mat           Data to estimate the HPI positions from
    * @param[out] transDevHead   The final dev head transformation matrix
    * @param[out] vGof           The goodness of fit in mm for each fitted HPI coil.
    * @param[in] vFreqs          The frequencies for each coil.
    * @param[in] fittedPointSet  The final fitted positions in form of a digitizer set.
    */
    void singleHPIFit(const Eigen::MatrixXd& t_mat,
                      FIFFLIB::FiffCoordTrans &transDevHead,
                      const QVector<int>& vFreqs,
                      QVector<double> &vGof,
                      FIFFLIB::FiffDigPointSet& fittedPointSet);

    //=========================================================================================================
    /**
    * Slot to receive incoming data.
    *
    * @param[in] p_DataSegment  Data to estimate the HPI positions from
    */
    void append(const Eigen::MatrixXd &p_DataSegment);

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

protected:
    //=========================================================================================================
    /**
    * Fits dipoles for the given coils and a given data set.
    *
    * @param[in] coilParam      The coil parameters.
    * @param[in] sensors        The sensor information.
    * @param[in] data           The data which used to fit the coils.
    * @param[in] numCoils       The number of coils.
    *
    * @return Returns the coil parameters.
    */
    coilParam dipfit(struct coilParam coil, struct sens sensors, Eigen::MatrixXd data, int numCoils);

    //=========================================================================================================
    /**
    * Computes the transformation matrix between two sets of 3D points.
    *
    * @param[in] NH     The first set of input 3D points (row-wise order).
    * @param[in] BT     The second set of input 3D points (row-wise order).
    *
    * @return Returns the transformation matrix.
    */
    Eigen::Matrix4d computeTransformation(Eigen::MatrixXd NH, Eigen::MatrixXd BT);

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pRawMatrixBuffer;    /**< The Circular Raw Matrix Buffer. */
    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;           /**< Holds the fiff measurement information. */

    QMutex              m_mutex;                /**< The global mutex to provide thread safety.*/

    bool                m_bIsRunning;           /**< Holds if real-time Covariance estimation is running.*/

    QString             m_sHPIResourceDir;      /**< Hold the resource folder to store the debug information in. */
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

#endif // RTHPIS_H
