//=============================================================================================================
/**
* @file     dummytoolbox.h
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
* @brief    Contains the declaration of the DummyToolbox class.
*
*/

#ifndef DUMMYTOOLBOX_H
#define DUMMYTOOLBOX_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummytoolbox_global.h"

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularbuffer.h>
#include <xMeas/newrealtimesamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DummyToolboxPlugin
//=============================================================================================================

namespace DummyToolboxPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS DummyToolbox
*
* @brief The DummyToolbox class provides a dummy algorithm structure.
*/
class DUMMYTOOLBOXSHARED_EXPORT DummyToolbox : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "dummytoolbox.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a DummyToolbox.
    */
    DummyToolbox();

    //=========================================================================================================
    /**
    * Destroys the DummyToolbox.
    */
    ~DummyToolbox();

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    void init();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

protected:
    virtual void run();

private:
    bool m_bIsRunning;

    PluginInputData<NewRealTimeSampleArray>::SPtr   m_pDummyInput;      /**< The RealTimeSampleArray of the DummyToolbox input.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr  m_pDummyOutput;    /**< The RealTimeSampleArray of the DummyToolbox output.*/

    dBuffer::SPtr   m_pDummyBuffer;      /**< Holds incoming data.*/
};

} // NAMESPACE

#endif // DUMMYTOOLBOX_H
