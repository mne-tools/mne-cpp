//=============================================================================================================
/**
* @file     {{about_template_header_filename}}
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
* @brief    Contains the declaration of the DummyAboutWidget class.
*
*/

#ifndef {{about_widget_header_define}}
#define {{about_widget_header_define}}


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_{{about_widget_header_filename}}"


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


//=============================================================================================================
/**
* DECLARE CLASS {{about_widget_name}}
*
* @brief The {{about_widget_name}} class provides the about dialog for the {{name}}.
*/
class {{about_widget_name}} : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a {{about_widget_name}} dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new {{about_widget_name}} becomes a window. If parent is another widget, {{about_widget_name}} becomes a child window inside parent. {{about_widget_name}} is deleted when its parent is deleted.
    */
    {{about_widget_name}}(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the {{about_widget_name}}.
    * All {{about_widget_name}}'s children are deleted first. The application exits if {{about_widget_name}} is the main widget.
    */
    ~{{about_widget_name}}();

private:

    Ui::{{about_widget_name}} ui;		/**< Holds the user interface for the {{about_widget_name}}.*/

};

} // NAMESPACE

#endif // {{about_widget_header_define}}
