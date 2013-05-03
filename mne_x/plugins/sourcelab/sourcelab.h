//=============================================================================================================
/**
* @file     sourcelab.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
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
* @brief    Contains the declaration of the SourceLab class.
*
*/

#ifndef SOURCELAB_H
#define SOURCELAB_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelab_global.h"
#include <mne_x/Interfaces/IRTAlgorithm.h>

#include <generics/circularmatrixbuffer.h>

#include <fs/annotationset.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_forwardsolution.h>
#include <inverse/sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>
#include <rtInv/rtcov.h>
#include <rtInv/rtinvop.h>
#include <rtInv/rtave.h>

#include <xMeas/Measurement/realtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SourceLabPlugin
//=============================================================================================================

namespace SourceLabPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace RTINVLIB;
using namespace MNEX;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS SourceLab
*
* @brief The SourceLab class provides a dummy algorithm structure.
*/
class SOURCELABSHARED_EXPORT SourceLab : public IRTAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "sourcelab.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IRTAlgorithm)

public:

    //=========================================================================================================
    /**
    * Constructs a SourceLab.
    */
    SourceLab();
    //=========================================================================================================
    /**
    * Destroys the SourceLab.
    */
    ~SourceLab();

    virtual bool start();
    virtual bool stop();

    virtual Type getType() const;
    virtual const char* getName() const;

    virtual QWidget* setupWidget();
    virtual QWidget* runWidget();

    virtual void update(Subject* pSubject);

//slot
    //=========================================================================================================
    /**
    * Append evoked
    *
    * @param[in] p_pEvoked  The evoked to be appended
    */
    void appendEvoked(FiffEvoked::SPtr p_pEvoked);

    //=========================================================================================================
    /**
    * Slot to update the fiff covariance
    *
    * @param[in] p_pFiffCov    The covariance to update
    */
    void updateFiffCov(FiffCov::SPtr p_pFiffCov);

    //=========================================================================================================
    /**
    * Slot to update the inverse operator
    *
    * @param[in] p_pInvOp    The inverse operator to update
    */
    void updateInvOp(MNEInverseOperator::SPtr p_pInvOp);

signals:
    //=========================================================================================================
    /**
    * Emits status messages
    *
    * @param[in] p_qStringMsg   The status message
    */
    void statMsg(QString p_qStringMsg);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the SourceLab.
    */
    void init();

    QMutex mutex;

    CircularMatrixBuffer<double>::SPtr m_pSourceLabBuffer;   /**< Holds incoming rt server data.*/

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bReceiveData;    /**< If thread is ready to receive data */

    //MNE stuff
    QFile m_qFileFwdSolution;           /**< File to forward solution. */
    MNEForwardSolution::SPtr m_pFwd;            /**< Forward solution. */
    MNEForwardSolution::SPtr m_pClusteredFwd;   /**< Clustered forward solution. */

    AnnotationSet m_annotationSet;  /**< Annotation set. */

    FiffInfo::SPtr m_pFiffInfo;     /**< Fiff information. */

    RtCov::SPtr m_pRtCov;           /**< Real-time covariance. */
    FiffCov::SPtr m_pFiffCov;       /**< The estimated covariance. */

    RtInvOp::SPtr m_pRtInvOp;           /**< Real-time inverse operator. */
    MNEInverseOperator::SPtr m_pInvOp;  /**< The inverse operator. */

    RtAve::SPtr m_pRtAve;                   /**< Real-time average. */
    QVector<FiffEvoked::SPtr> m_qVecEvokedData; /**< Evoked data set */


    MinimumNorm::SPtr m_pMinimumNorm;   /**< Minimum Norm Estimation. */
};

} // NAMESPACE

#endif // SOURCELAB_H
