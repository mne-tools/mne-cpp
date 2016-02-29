//=============================================================================================================
/**
* @file     gusbamp.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the GUSBAmp class.
*
*/




#ifndef GUSBAMP_H
#define GUSBAMP_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "gusbamp_global.h"
#include <Windows.h>

#include <fstream>
#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>

#include "FormFiles/gusbampsetupwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GUSBAmpPlugin
//=============================================================================================================

namespace GUSBAmpPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;
using namespace FIFFLIB;
using namespace std;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmpProducer;


//=============================================================================================================
/**
* GUSBAmp...
*
* @brief The GUSBAmp class provides an EEG connector for the gTec USBAmp device.
*/
class GUSBAMPSHARED_EXPORT GUSBAmp : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "gusbamp.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class GUSBAmpProducer;
    friend class GUSBAmpSetupWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a GUSBAmp.
    */
    GUSBAmp();

    //=========================================================================================================
    /**
    * Destroys the GUSBAmp.
    */
    virtual ~GUSBAmp();

    //=========================================================================================================
    /**
    * building all setting for the FIFF-data-stream
    */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the GUSBAmp by starting the GUSBAmp's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the GUSBAmp by stopping the GUSBAmp's thread.
    */
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr m_pRTMSA_GUSBAmp;   /**< The RealTimeSampleArray to provide the EEG data.*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    bool                                m_bIsRunning;                       /**< Whether GUSBAmp is running.*/

    QSharedPointer<RawMatrixBuffer>     m_pRawMatrixBuffer_In;              /**< Holds incoming raw data.*/

    QSharedPointer<GUSBAmpProducer>     m_pGUSBAmpProducer;                 /**< the GUSBAmpProducer.*/

    QSharedPointer<FiffInfo>            m_pFiffInfo;                        /**< Fiff measurement info.*/

    vector<QString>     m_vSerials;                 /**< vector of all Serials (the first one is the master) */
    int                 m_iSampleRate;              /**< the sample rate in Hz (see documentation of the g.USBamp API for details on this value and the NUMBER_OF_SCANS!)*/
    int                 m_iSamplesPerBlock;         /**< The samples per block defined by the user via the GUI. */
    UCHAR               m_iNumberOfChannels;        /**< the channels that should be acquired from each device */
    QString             m_sFilePath;                /**< String of the Filepath where acquisition data will be stored */
    vector<int>         m_viSizeOfSampleMatrix;     /**< vector including the size of the two dimensional sample Matrix */
    vector<int>         m_viChannelsToAcquire;      /**< vector of the calling numbers of the channels to be acquired */
    GUSBAmpSetupWidget* m_pWidget;
};

} // NAMESPACE

#endif // GUSBAMP_H
