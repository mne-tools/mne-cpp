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

using namespace GUSBAMPPLUGIN;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupWidget::GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent)
: QWidget(parent)
, m_pGUSBAmp(pGUSBAmp)
{
    ui.setupUi(this);

    //Connections
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &GUSBAmpSetupWidget::showAboutDialog);
    connect(ui.m_pushButton_setSerials, &QPushButton::clicked, this, &GUSBAmpSetupWidget::setSerialAdresses);
    connect(ui.ChannelSelect, &QGroupBox::clicked, this, &GUSBAmpSetupWidget::activateChannelSelect);

    ui.comboBox->setCurrentIndex(2);
}


//*************************************************************************************************************

GUSBAmpSetupWidget::~GUSBAmpSetupWidget()
{
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::initGui()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::showAboutDialog()
{
    GUSBAmpAboutWidget aboutDialog(this);
    aboutDialog.exec();
}

//*************************************************************************************************************

void GUSBAmpSetupWidget::on_comboBox_currentIndexChanged(const QString &arg1)
{
    bool ok;
    QString sampleRate = arg1;
    m_pGUSBAmp->m_iSampleRate = sampleRate.toInt(&ok,10); //QString to int
}

//*************************************************************************************************************

void GUSBAmpSetupWidget::setSerialAdresses()
{
    vector<QString> serials;

    QString master = ui.master->text();
    QString slave1 = ui.slave1->text();
    QString slave2 = ui.slave2->text();
    QString slave3 = ui.slave3->text();

    if(master.isEmpty())
    {
        QMessageBox::information(this,"ERROR - master serial", "ERROR set master: field master is not supposed to be empty!");
        return;
    }
    //master has to be first in the list
    ui.label->setText(master);
    serials.push_back(master);

    if(!slave1.isEmpty())
    {
        ui.label1->setText(slave1);
        serials.push_back(slave1);
    }
    else
        ui.label1->clear();
    if(!slave2.isEmpty())
    {
        ui.label2->setText(slave2);
        serials.push_back(slave2);
    }
    else
        ui.label2->clear();
    if(!slave3.isEmpty())
    {
        ui.label3->setText(slave3);
        serials.push_back(slave3);
    }
    else
        ui.label3->clear();

    int size = serials.size();

    m_pGUSBAmp->m_vSerials.resize(size);
    m_pGUSBAmp->m_vSerials = serials;
}

//*************************************************************************************************************

void GUSBAmpSetupWidget::checkBoxes()
{
    vector<int> list;

    if(ui.checkBox->isChecked())
        list.push_back(1);

    if(ui.checkBox_2->isChecked())
        list.push_back(2);

    if(ui.checkBox_3->isChecked())
        list.push_back(3);

    if(ui.checkBox_4->isChecked())
        list.push_back(4);

    if(ui.checkBox_5->isChecked())
        list.push_back(5);

    if(ui.checkBox_6->isChecked())
        list.push_back(6);

    if(ui.checkBox_7->isChecked())
        list.push_back(7);

    if(ui.checkBox_8->isChecked())
        list.push_back(8);

    if(ui.checkBox_9->isChecked())
        list.push_back(9);

    if(ui.checkBox_10->isChecked())
        list.push_back(10);

    if(ui.checkBox_11->isChecked())
        list.push_back(11);

    if(ui.checkBox_12->isChecked())
        list.push_back(12);

    if(ui.checkBox_13->isChecked())
        list.push_back(13);

    if(ui.checkBox_14->isChecked())
        list.push_back(14);

    if(ui.checkBox_15->isChecked())
        list.push_back(15);

    if(ui.checkBox_16->isChecked())
        list.push_back(16);

    if(!list.empty())
    {
    //store the channels-to-acquire-list in the according member variable
    m_pGUSBAmp->m_viChannelsToAcquire.resize(list.size());
    m_pGUSBAmp->m_viChannelsToAcquire = list;
    }
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::activateChannelSelect(bool checked)
{
    if(!checked)
    {
        ui.checkBox->setChecked(true);
        ui.checkBox_2->setChecked(true);
        ui.checkBox_3->setChecked(true);
        ui.checkBox_4->setChecked(true);
        ui.checkBox_5->setChecked(true);
        ui.checkBox_6->setChecked(true);
        ui.checkBox_7->setChecked(true);
        ui.checkBox_8->setChecked(true);
        ui.checkBox_9->setChecked(true);
        ui.checkBox_10->setChecked(true);
        ui.checkBox_11->setChecked(true);
        ui.checkBox_12->setChecked(true);
        ui.checkBox_13->setChecked(true);
        ui.checkBox_14->setChecked(true);
        ui.checkBox_15->setChecked(true);
        ui.checkBox_16->setChecked(true);

        checkBoxes();
    }

    if(checked)
    {
        ui.checkBox->setChecked(false);
        ui.checkBox_2->setChecked(false);
        ui.checkBox_3->setChecked(false);
        ui.checkBox_4->setChecked(false);
        ui.checkBox_5->setChecked(false);
        ui.checkBox_6->setChecked(false);
        ui.checkBox_7->setChecked(false);
        ui.checkBox_8->setChecked(false);
        ui.checkBox_9->setChecked(false);
        ui.checkBox_10->setChecked(false);
        ui.checkBox_11->setChecked(false);
        ui.checkBox_12->setChecked(false);
        ui.checkBox_13->setChecked(false);
        ui.checkBox_14->setChecked(false);
        ui.checkBox_15->setChecked(false);
        ui.checkBox_16->setChecked(false);

        checkBoxes();
    }
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_2_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_3_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_4_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_5_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_6_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_7_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_8_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_9_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_10_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_11_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_12_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_13_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_14_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_15_clicked()
{
    checkBoxes();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::on_checkBox_16_clicked()
{
    checkBoxes();
}
