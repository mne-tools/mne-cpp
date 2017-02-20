//=============================================================================================================
/**
* @file     referencesetupwidget.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ReferenceSetupWidget class.
*
*/

#ifndef REFERENCESETUPWIDGET_H
#define REFERENCESETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "reference_global.h"
#include "../ui_referencesetup.h"
#include "referenceaboutwidget.h"
#include "../reference.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REFERENCEPLUGIN
//=============================================================================================================

namespace REFERENCEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Reference;


//=============================================================================================================
/**
* DECLARE CLASS ReferenceSetupWidget
*
* @brief The ReferenceSetupWidget class provides the Reference configuration window.
*/
class REFERENCESHARED_EXPORT ReferenceSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ReferenceSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding ReferenceToolbox.
    * @param [in] parent pointer to parent widget; If parent is 0, the new ReferenceSetupWidget becomes a window. If parent is another widget, ReferenceSetupWidget becomes a child window inside parent. ReferenceSetupWidget is deleted when its parent is deleted.
    */
    ReferenceSetupWidget(Reference* pRef, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ReferenceSetupWidget.
    * All ReferenceSetupWidget's children are deleted first. The application exits if ReferenceSetupWidget is the main widget.
    */
    ~ReferenceSetupWidget();

private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:
    Reference* m_pRef;	/**< Holds a pointer to corresponding Reference object.*/

    Ui::ReferenceSetupWidget ui;	/**< Holds the user interface for the ReferenceSetupWidget.*/
};

} // NAMESPACE

#endif // REFERENCESETUPWIDGET_H
