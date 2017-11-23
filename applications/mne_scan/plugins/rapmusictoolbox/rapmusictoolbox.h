//=============================================================================================================
/**
* @file     rapmusictoolbox.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the RapMusicToolbox class.
*
*/

#ifndef RAPMUSICTOOLBOX_H
#define RAPMUSICTOOLBOX_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rapmusictoolbox_global.h"
#include <scShared/Interfaces/IAlgorithm.h>

#include <utils/generics/circularmatrixbuffer.h>

#include <fs/annotationset.h>
#include <fs/surfaceset.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_forwardsolution.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/rapMusic/pwlrapmusic.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeevoked.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RapMusicToolboxPlugin
//=============================================================================================================

namespace RapMusicToolboxPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS RapMusicToolbox
*
* @brief The RapMusicToolbox class provides a dummy algorithm structure.
*/
class RAPMUSICTOOLBOXSHARED_EXPORT RapMusicToolbox : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rapmusictoolbox.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class RapMusicToolboxSetupWidget;

public:

    //=========================================================================================================
    /**
    * Constructs a RapMusicToolbox.
    */
    RapMusicToolbox();
    //=========================================================================================================
    /**
    * Destroys the RapMusicToolbox.
    */
    ~RapMusicToolbox();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise the RapMusicToolbox.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    void calcFiffInfo();

    void doClustering();

    void finishedClustering();

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Slot to update the fiff evoked
    *
    * @param[in] pMeasurement   The evoked to be appended
    */
    void updateRTE(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

signals:
    //=========================================================================================================
    /**
    * Signal when clsutering is started
    */
    void clusteringStarted();

    //=========================================================================================================
    /**
    * Signal when clsutering has finished
    */
    void clusteringFinished();

protected:
    virtual void run();

private:
    PluginInputData<RealTimeEvoked>::SPtr   m_pRTEInput;    /**< The RealTimeEvoked input.*/

    PluginOutputData<RealTimeSourceEstimate>::SPtr      m_pRTSEOutput;  /**< The RealTimeSourceEstimate output.*/

    QMutex m_qMutex;

    QVector<FiffEvoked> m_qVecFiffEvoked;
    qint32 m_iNumAverages;

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bReceiveData;    /**< If thread is ready to receive data */
    bool m_bProcessData;    /**< If data should be received for processing */

    //RapMusicToolbox stuff
    QFile                       m_qFileFwdSolution; /**< File to forward solution. */
    MNEForwardSolution::SPtr    m_pFwd;             /**< Forward solution. */
    MNEForwardSolution::SPtr    m_pClusteredFwd;    /**< Clustered forward solution. */

    bool m_bFinishedClustering;                     /**< If clustered forward solution is available. */

    QString                     m_sAtlasDir;        /**< File to Atlas. */
    AnnotationSet::SPtr         m_pAnnotationSet;   /**< Annotation set. */
    QString                     m_sSurfaceDir;      /**< File to Surface. */
    SurfaceSet::SPtr            m_pSurfaceSet;      /**< Surface set. */

    FiffInfo::SPtr              m_pFiffInfo;        /**< Fiff information. */
    FiffInfo::SPtr              m_pFiffInfoEvoked;  /**< Fiff information of the evoked. */
    FiffInfoBase::SPtr          m_pFiffInfoForward; /**< Fiff information of the forward solution. */

    QStringList                 m_qListPickChannels;        /**< Channels to pick */

    PwlRapMusic::SPtr           m_pPwlRapMusic;     /**< Powell RAP MUSIC. */
    qint32                      m_iDownSample;      /**< Sampling rate */

//    RealTimeSourceEstimate::SPtr m_pRTSE_MNE; /**< Source Estimate output channel. */
};

} // NAMESPACE

#endif // RAPMUSICTOOLBOX_H
