//=============================================================================================================
/**
* @file     {{header_filename}}
* @author   {{author}} <{{author_email}}>
*           Erik Hornberger <erik.hornberger@shi-g.com>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     {{month}}, {{year}}
*
* @section  LICENSE
*
* Copyright (C) {{year}}, {{author}}, Christoph Dinh, and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the {{name}} class.
*
*/

#ifndef {{header_define}}
#define {{header_define}}


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "{{global_header_filename}}"

#include <scShared/Interfaces/{{superclass}}.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimemultisamplearray.h>
#include "FormFiles/{{setup_widget_header_filename}}"
#include "FormFiles/{{widget_header_filename}}"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE {{name}}
//=============================================================================================================

namespace {{namespace}}
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
* DECLARE CLASS {{name}}
*
* @brief The {{name}} class does...
*/
class {{export_define}} {{name}} : public {{superclass}}
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "{{json_filename}}") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::{{superclass}})

public:
    //=========================================================================================================
    /**
    * Constructs a {{name}}.
    */
    {{name}}();

    //=========================================================================================================
    /**
    * Destroys the {{name}}.
    */
    ~{{name}}();

    //=========================================================================================================
    /**
    * {{superclass}} functions
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
    * Udates the plugin with new (incoming) data.
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
    */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
    * {{superclass}} function
    */
    virtual void run();

    /**
    * Shows the primary widget
    */
    void showWidget();

private:
    bool                                            m_bIsRunning;           /**< Flag whether thread is running.*/
    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;            /**< Fiff measurement info.*/
    QSharedPointer<{{widget_name}}>                 m_pWidget;              /**< flag whether thread is running.*/
    QAction*                                        m_pActionShowWidget;    /**< Action for showing the widget.*/
    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pBuffer;              /**< Holds incoming data.*/

    PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr      m_pInput;      /**< The RealTimeMultiSampleArray of the {{name}} input.*/
    PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pOutput;     /**< The RealTimeMultiSampleArray of the {{name}} output.*/

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // {{header_define}}
