//=============================================================================================================
/**
* @file     neuronalconnectivityaboutwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the NeuronalConnectivityAboutWidget class.
*
*/

#ifndef NEURONALCONNECTIVITYABOUTWIDGET_H
#define NEURONALCONNECTIVITYABOUTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_neuronalconnectivityabout.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEURONALCONNECTIVITYPLUGIN
//=============================================================================================================

namespace NEURONALCONNECTIVITYPLUGIN
{


//=============================================================================================================
/**
* DECLARE CLASS NeuronalConnectivityAboutWidget
*
* @brief The NeuronalConnectivityAboutWidget class provides the about dialog for the NeuronalConnectivity plugin.
*/
class NeuronalConnectivityAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a NeuronalConnectivityAboutWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new NeuronalConnectivityAboutWidget becomes a window. If parent is another widget, NeuronalConnectivityAboutWidget becomes a child window inside parent. NeuronalConnectivityAboutWidget is deleted when its parent is deleted.
    */
    NeuronalConnectivityAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the NeuronalConnectivityAboutWidget.
    * All NeuronalConnectivityAboutWidget's children are deleted first. The application exits if NeuronalConnectivityAboutWidget is the main widget.
    */
    ~NeuronalConnectivityAboutWidget();

private:

    Ui::NeuronalConnectivityAboutWidgetClass ui;		/**< Holds the user interface for the NeuronalConnectivityAboutWidget.*/

};

} // NAMESPACE

#endif // NEURONALCONNECTIVITYABOUTWIDGET_H
