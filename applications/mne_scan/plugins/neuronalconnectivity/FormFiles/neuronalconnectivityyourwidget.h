//=============================================================================================================
/**
* @file     neuronalconnectivitytoolbox.h
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
* @brief    Contains the declaration of the NeuronalConnectivityYourWidget class.
*
*/

#ifndef NEURONALCONNECTIVITYYOURWIDGET_H
#define NEURONALCONNECTIVITYYOURWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_neuronalconnectivityyourtoolbarwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEURONALCONNECTIVITYPLUGIN
//=============================================================================================================

namespace NEURONALCONNECTIVITYPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS NeuronalConnectivityYourWidget
*
* @brief The NeuronalConnectivityToolbox class provides a NeuronalConnectivity plugin widget structure.
*/
class NeuronalConnectivityYourWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NeuronalConnectivityYourWidget> SPtr;         /**< Shared pointer type for NeuronalConnectivityYourWidget. */
    typedef QSharedPointer<NeuronalConnectivityYourWidget> ConstSPtr;    /**< Const shared pointer type for NeuronalConnectivityYourWidget. */

    //=========================================================================================================
    /**
    * Constructs a NeuronalConnectivityToolbox.
    */
    NeuronalConnectivityYourWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the NeuronalConnectivityToolbox.
    */
    ~NeuronalConnectivityYourWidget();

private:
    Ui::NeuronalConnectivityYourToolbarWidget* ui;        /**< The UI class specified in the designer. */

};
}
#endif // NEURONALCONNECTIVITYYOURWIDGET_H
