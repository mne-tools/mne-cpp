//=============================================================================================================
/**
* @file     {{setup_widget_header_filename}}
* @author   {{author}} <{{author_email}}>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     {{month}}, {{year}}
*
* @section  LICENSE
*
* Copyright (C) {{year}}, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the {{setup_widget_name}} class.
*
*/

#ifndef {{setup_widget_header_define}}
#define {{setup_widget_header_define}}


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_{{setup_widget_header_filename}}"
#include "{{about_widget_header_filename}}"
#include "../{{header_filename}}"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE {{namespace}}
//=============================================================================================================

namespace {{namespace}}
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class {{name}};


//=============================================================================================================
/**
* DECLARE CLASS {{setup_widget_name}}
*
* @brief The {{setup_widget_name}} class provides the {{name}} configuration window.
*/
class {{setup_widget_name}} : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a {{setup_widget_name}} which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding {{name}}.
    * @param [in] parent pointer to parent widget; If parent is 0, the new DummySetupWidget becomes a window. If parent is another widget, {{setup_widget_name}} becomes a child window inside parent. {{setup_widget_name}} is deleted when its parent is deleted.
    */
    {{setup_widget_name}}({{name}}* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the {{setup_widget_name}}.
    * All {{setup_widget_name}}'s children are deleted first. The application exits if {{setup_widget_name}} is the main widget.
    */
    ~{{setup_widget_name}}();


private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:

    {{name}}* m_pToolbox;	/**< Holds a pointer to corresponding {{name}}.*/

    Ui::{{setup_widget_name}} ui;	/**< Holds the user interface for the {{setup_widget_name}}.*/
};

} // NAMESPACE

#endif // {{setup_widget_header_define}}
