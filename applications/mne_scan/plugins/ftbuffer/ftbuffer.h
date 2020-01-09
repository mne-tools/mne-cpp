/**
* @file     ftbuffer.h
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the FtBuffer class.
*
*/

#ifndef FTBUFFER_H
#define FTBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// QT Includes
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QSharedPointer>

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"
#include "FormFiles/ftbufferaboutwidget.h"
#include "FormFiles/ftbuffersetupwidget.h"
#include "FormFiles/ftbufferyourwidget.h"

#include <scShared/Interfaces/ISensor.h>
#include <scShared/Interfaces/IAlgorithm.h>

#include <ftsrc/ftbuffclient.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN {


class FTBUFFER_EXPORT FtBuffer : public SCSHAREDLIB::ISensor
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ftbuffer.json")

    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class FtBufferSetupWidget;
public:

    FtBuffer();

    ~FtBuffer();

    //ISENSOR Functions
    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initializes the plugin.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns the plugin type.
    * Pure virtual method inherited by IModule.
    *
    * @return type of the ISensor
    */
    virtual PluginType getType() const;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the ISensor.
    */
    virtual QString getName() const;


    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Changes stored address and connects the member FtBuffClient to that address
    *
    * @return connects buffer client to provided address
    */
    void connectToBuffer(QString addr);


protected:
    virtual void run();

    void showYourWidget();

private:
    bool                                            m_bIsRunning;

    QSharedPointer<FtBufferYourWidget>              m_pYourWidget;
    QAction*                                        m_pActionShowYourWidget;

    FtBuffClient                                    m_FtBuffClient;

};

}//namespace end brace

#endif // FTBUFFER_H
