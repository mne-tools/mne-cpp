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
#include <mne/mne_forwardsolution.h>
#include <rtInv/rtcov.h>

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
using namespace MNEX;
using namespace RTINVLIB;
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

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the SourceLab.
    */
    void init();

    QMutex mutex;

    _double_CircularMatrixBuffer* m_pSourceLabBuffer;   /**< Holds incoming rt server data.*/

    bool m_bIsRunning;  /**< If source lab is running */

    //MNE stuff
    QFile m_qFileFwdSolution;           /**< File to forward solution. */
    MNEForwardSolution m_Fwd;           /**< Forward solution. */
    MNEForwardSolution m_clusteredFwd;  /**< Clustered forward solution. */

    AnnotationSet m_annotationSet;  /**< Annotation set. */

    FiffInfo::SPtr m_pFiffInfo;    /**< Fiff information. */

    RtCov::SPtr m_pRtCov;          /**< Real time covariance. */
};

} // NAMESPACE

#endif // SOURCELAB_H
