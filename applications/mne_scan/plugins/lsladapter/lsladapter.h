//=============================================================================================================
/**
* @file     lsladapter.h
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     February, 2019
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the LSLAdapter class.
*
*/

#ifndef LSLADAPTER_H
#define LSLADAPTER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter_global.h"

#include <scShared/Interfaces/ISensor.h>

#include <lsl_cpp.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QThread>
#include <QFutureWatcher>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QListWidgetItem;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE LSLADAPTERPLUGIN
//=============================================================================================================

namespace LSLADAPTERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// LSLADAPTERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class LSLAdapterProducer;


//=============================================================================================================
/**
* The LSL class deals with the lsl library (labstreaminglayer)
*/
class LSLADAPTERSHARED_EXPORT LSLAdapter : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "lsladapter.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

public:
    //=========================================================================================================
    /**
    * Constructs LSLAdapter.
    */
    LSLAdapter();

    //=========================================================================================================
    /**
    * Destroys LSL.
    */
    virtual ~LSLAdapter();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    virtual void init();

    virtual void unload();

    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the LSL adapter by stopping its background thread
    */
    virtual bool stop();

    virtual QWidget* setupWidget();

    virtual inline IPlugin::PluginType getType() const;

    virtual inline QString getName() const;

public slots:

    //=========================================================================================================
    /**
    * This is called by the UI, whenever the user wants to manually refresh the list of available LSL streams.
    */
    void onRefreshAvailableStreams();

    //=========================================================================================================
    /**
    * This is called by the UI via a connect. It retrieves the LSL stream that corresponds to the passed
    * QListWidgetItem and starts the background thread.
    */
    void onStartStream(const lsl::stream_info& stream);

    //=========================================================================================================
    /**
    * This is called by the UI via a connect. It stops the background thread / disconnects from the LSL stream.
    */
    void onStopStream();

    //=========================================================================================================
    /**
    * This is called by the background thread via a connect. It allows us to perform additional cleanup after
    * the thread has finished.
    */
    void onProducerThreadFinished();

protected:

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

signals:

    //=========================================================================================================
    /**
    * This is emitted in order to tell the UI that the list of available LSL streams has been updated.
    */
    void updatedAvailableLSLStreams(QVector<lsl::stream_info>& vStreamInfos);

private slots:

    //=========================================================================================================
    /**
    * This is called by the QFutureWatcher, indicating that the background scanning is complete.
    */
    void onLSLStreamScanReady();

private:

    //=========================================================================================================
    /**
    * Helper function for getting a list of LSL streams that fulfill the current filtering settings.
    */
    static QVector<lsl::stream_info> scanAvailableLSLStreams();


    QFutureWatcher<QVector<lsl::stream_info> >      m_updateStreamsFutureWatcher;
    QVector<lsl::stream_info>                       m_vAvailableStreams;
    QThread                                         m_pProducerThread;
    LSLAdapterProducer*                             m_pProducer;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline SCSHAREDLIB::IPlugin::PluginType LSLAdapter::getType() const
{
    return _ISensor;
}


//************************************************************************************************************

inline QString LSLAdapter::getName() const
{
    return "LSL Adapter";
}

} // NAMESPACE

#endif // LSLADAPTER_H
