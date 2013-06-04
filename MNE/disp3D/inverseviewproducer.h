//=============================================================================================================
/**
* @file     inverseviewproducer.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    InverseViewProducer class declaration
*
*/

#ifndef INVERSEVIEWPRODUCER_H
#define INVERSEVIEWPRODUCER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/sourceestimate.h>


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class InverseView;

//=============================================================================================================
/**
* Functionality of a timer
*
* @brief 3D stereoscopic labels
*/
class InverseViewProducer : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<InverseViewProducer> SPtr;            /**< Shared pointer type for InverseView class. */
    typedef QSharedPointer<const InverseViewProducer> ConstSPtr; /**< Const shared pointer type for InverseView class. */
    
    //=========================================================================================================
    /**
    * Default constructor
    *
    *
    * @param[in] p_iT   Time in us between each sample (default 1000)
    */
    InverseViewProducer(qint32 p_iT = 1000);
    
    //=========================================================================================================
    /**
    * Destroys the InverseView class.
    */
    ~InverseViewProducer();

    //=========================================================================================================
    /**
    * Returns the maximal activation.
    *
    * @return the maximal activation.
    */
    inline double getGlobalMax() const;

    //=========================================================================================================
    /**
    * Returns the maximal activation of each source.
    *
    * @return the maximal activation source vector.
    */
    inline VectorXd getMaxActivation() const;

    //=========================================================================================================
    /**
    * Appends a new source estimate
    *
    * @param[in] p_sourceEstimate   Source estimate to push
    */
    void pushSourceEstimate(SourceEstimate &p_sourceEstimate);

    //=========================================================================================================
    /**
    * Stops the InverseViewProducer by stopping the producer's thread.
    */
    void stop();

signals:
    void sourceEstimateSample(QSharedPointer<Eigen::VectorXd>);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();


private:
    QMutex mutex;

    bool m_bIsRunning;      /**< If inverse view producer is running. */

    SourceEstimate m_curSourceEstimate; /**< Current source estimate.*/

    VectorXd m_vecMaxActivation;      /**< Maximum of each source. */
    double m_dGlobalMaximum;             /**< Global maximum. */

    qint32 m_iFps;          /**< Frames per second.*/
    qint32 m_iT;            /**< Time in us between each step.*/
    qint32 m_nTimeSteps;    /**< Number of time steps.*/

    qint32 m_iDownSampling; /**< Downsampling factor */

    bool m_bBeep;           /**< Indicate stimulus onset with a beep tone. */

//    CircularMatrixBuffer<double>::SPtr m_pSourceEstimateBuffer; /**< Holds incoming source estimate sample data.*/



//    RowVectorXd m_dMaxActivation;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

double InverseViewProducer::getGlobalMax() const
{
    return m_dGlobalMaximum;
}


//*************************************************************************************************************

VectorXd InverseViewProducer::getMaxActivation() const
{
    return m_vecMaxActivation;
}


} // NAMESPACE

#endif // INVERSEVIEWPRODUCER_H

