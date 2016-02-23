//=============================================================================================================
/**
* @file     gusbampsetupwidget.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the GUSBAmpSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampsetupwidget.h"
#include "gusbampaboutwidget.h"
#include "../gusbamp.h"




//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAmpPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupWidget::GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent)
: QWidget(parent)
, m_pGUSBAmp(pGUSBAmp)
{
    ui.setupUi(this);

}


//*************************************************************************************************************

GUSBAmpSetupWidget::~GUSBAmpSetupWidget()
{

}


//*************************************************************************************************************

void GUSBAmpSetupWidget::initGui()
{

}


//*************************************************************************************************************

//void GUSBAmpSetupWidget::showDialog()
//{
//    GUSBAmpAboutWidget aboutDialog(this);
//    aboutDialog.exec();
//}

//*************************************************************************************************************



void GUSBAmpPlugin::GUSBAmpSetupWidget::on_comboBox_activated(const QString &arg1)
{

    QMessageBox::information(this,"Sample Rate", arg1 );
    m_pGUSBAmp->m_iSampleRate = arg1.toInt();




}




void GUSBAmpPlugin::GUSBAmpSetupWidget::on_pushButton_clicked()
{
    int serialSize = 1;

    QString master = (ui.master->text());
    QString slave1 = (ui.slave1->text());
    QString slave2 = (ui.slave2->text());
    QString slave3 = (ui.slave3->text());

    ui.label->setText(master);
    ui.label1->setText(slave1);
    ui.label2->setText(slave2);
    ui.label3->setText(slave3);

    if(master.isEmpty())
    {
        QMessageBox::information(this,"ERROR - master serial", "ERROR set master: field master is not supposed to be empty!");
        return;
    }

    serialSize += !slave1.isEmpty() + !slave2.isEmpty() + !slave3.isEmpty();











}
