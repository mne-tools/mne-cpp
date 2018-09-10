//=============================================================================================================
/**
* @file     {{widget_header_filename}}
* @author   {{author}} <{{author_email}}>
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     {{month}}, {{year}}
*
* @section  LICENSE
*
* Copyright (C) {{year}}, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the DummyYourWidget class.
*
*/

#ifndef {{widget_header_define}}
#define {{widget_header_define}}

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_{{widget_header_filename}}"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


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


//=============================================================================================================
/**
* DECLARE CLASS DummyYourWidget
*
* @brief The DummyToolbox class provides a dummy toolbar widget structure.
*/
class {{widget_name}} : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<{{widget_name}}> SPtr;         /**< Shared pointer type for DummyYourWidget. */
    typedef QSharedPointer<{{widget_name}}> ConstSPtr;    /**< Const shared pointer type for DummyYourWidget. */

    //=========================================================================================================
    /**
    * Constructs a {{widget_name}}.
    */
    explicit {{widget_name}}(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the {{widget_name}}.
    */
    ~{{widget_name}}();

private:
    Ui::{{widget_name}}* ui;        /**< The UI class specified in the designer. */
};

}   //namespace

#endif // {{widget_header_define}}
