//=============================================================================================================
/**
* @file     mnebuffer.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;;
*           Eric Larson <larson.eric.d@gmail.com>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh, Eric Larson and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the MneBuffer class.
*
*/

#ifndef MNEBUFFER_H
#define MNEBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnebuffer_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include "FormFiles/mnebuffersetupwidget.h"
#include "FormFiles/mnebufferwidget.h"

#include "sender.h"
#include "receiver.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBUFFERPLUGIN
//=============================================================================================================

namespace MNEBUFFERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* The MneBuffer provides a raw data streamer, whereas the API is compatible to the  FieldTrip buffer
*
* @brief The MneBuffer class provides a raw data streamer.
*/
class MNEBUFFERSHARED_EXPORT MneBuffer : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "mnebuffer.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a MneBuffer.
    */
    MneBuffer();

    //=========================================================================================================
    /**
    * Destroys the MneBuffer.
    */
    ~MneBuffer();

    //=========================================================================================================
    /**
    * IAlgorithm functions
    */
    virtual QSharedPointer<IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Udates the pugin with new (incoming) data.
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

protected:
    //=========================================================================================================
    /**
    * IAlgorithm function
    */
    virtual void run();

    void showMneBufferWidget();

private:
    bool                                            m_bIsRunning;           /**< Flag whether thread is running.*/

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<MneBufferWidget>                 m_pMneBufferWidget;             /**< flag whether thread is running.*/
    QAction*                                        m_pActionShowMneBufferWidget;   /**< flag whether thread is running.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pDataBuffer;          /**< Holds incoming data.*/

    PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pMneBufferInput;      /**< The NewRealTimeMultiSampleArray of the DummyToolbox input.*/
    PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr     m_pMneBufferOutput;     /**< The NewRealTimeMultiSampleArray of the DummyToolbox output.*/

    QSharedPointer<Sender> sender;
    QSharedPointer<Receiver> receiver;
};

} // NAMESPACE

#endif // MNEBUFFER_H
