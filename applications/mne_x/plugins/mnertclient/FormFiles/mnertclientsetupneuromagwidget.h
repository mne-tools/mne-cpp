//=============================================================================================================
/**
* @file     mnertclientsetupneuromagwidget.h
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
* @brief    Contains the declaration of the MneRtClientSetupNeuromagWidget class.
*
*/

#ifndef MNERTCLIENTSETUPNEUROMAGWIDGET_H
#define MNERTCLIENTSETUPNEUROMAGWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class MneRtClientSetupNeuromagWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MneRtClientPlugin
//=============================================================================================================

namespace MneRtClientPlugin
{

//=============================================================================================================
/**
* DECLARE CLASS MneRtClientSetupNeuromagWidget
*
* @brief The MneRtClientSetupNeuromagWidget class provides the Neuromag configuration window.
*/
class MneRtClientSetupNeuromagWidget : public QWidget
{
    Q_OBJECT
public:
    typedef QSharedPointer<MneRtClientSetupNeuromagWidget> SPtr;              /**< Shared pointer type for MneRtClientSetupNeuromagWidget. */
    typedef QSharedPointer<const MneRtClientSetupNeuromagWidget> ConstSPtr;   /**< Const shared pointer type for MneRtClientSetupNeuromagWidget. */

    explicit MneRtClientSetupNeuromagWidget(QWidget *parent = 0);
    ~MneRtClientSetupNeuromagWidget();
    
private:
    Ui::MneRtClientSetupNeuromagWidget *ui;
};

} // NAMESPACE

#endif // MNERTCLIENTSETUPNEUROMAGWIDGET_H
