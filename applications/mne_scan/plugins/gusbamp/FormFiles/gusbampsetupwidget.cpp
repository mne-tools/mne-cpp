//=============================================================================================================
/**
 * @file     gusbampsetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Viktor Klueber, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampsetupwidget.h"
#include "gusbampaboutwidget.h"
#include "../gusbamp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAMPPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupWidget::GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent)
: QWidget(parent)
, m_pGUSBAmp(pGUSBAmp)
{
    ui.setupUi(this);

    // connect push buttones and group boxes
    connect(ui.m_pushButton_setSerials, &QPushButton::clicked, this, &GUSBAmpSetupWidget::setSerialAdresses);
    connect(ui.ChannelSelect, &QGroupBox::clicked, this, &GUSBAmpSetupWidget::activateChannelSelect);

    // connect check boxes
    connect(ui.m_checkBox_channel1, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel1);
    connect(ui.m_checkBox_channel2, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel2);
    connect(ui.m_checkBox_channel3, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel3);
    connect(ui.m_checkBox_channel4, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel4);
    connect(ui.m_checkBox_channel5, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel5);
    connect(ui.m_checkBox_channel6, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel6);
    connect(ui.m_checkBox_channel7, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel7);
    connect(ui.m_checkBox_channel8, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel8);
    connect(ui.m_checkBox_channel9, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel9);
    connect(ui.m_checkBox_channel10, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel10);
    connect(ui.m_checkBox_channel11, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel11);
    connect(ui.m_checkBox_channel12, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel12);
    connect(ui.m_checkBox_channel13, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel13);
    connect(ui.m_checkBox_channel14, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel14);
    connect(ui.m_checkBox_channel15, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel15);
    connect(ui.m_checkBox_channel16, &QCheckBox::clicked, this, &GUSBAmpSetupWidget::activateChannel16);

    ui.comboBox->setCurrentIndex(2);
}

//=============================================================================================================

GUSBAmpSetupWidget::~GUSBAmpSetupWidget()
{
}

//=============================================================================================================

void GUSBAmpSetupWidget::initGui()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::showAboutDialog()
{
    GUSBAmpAboutWidget aboutDialog(this);
    aboutDialog.exec();
}

//=============================================================================================================

void GUSBAmpSetupWidget::on_comboBox_currentIndexChanged(const QString &arg1)
{
    bool ok;
    QString sampleRate = arg1;
    m_pGUSBAmp->m_iSampleRate = sampleRate.toInt(&ok,10); //QString to int
}

//=============================================================================================================

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

//=============================================================================================================

void GUSBAmpSetupWidget::checkBoxes()
{
    vector<int> list;

    if(ui.m_checkBox_channel1->isChecked())
        list.push_back(1);

    if(ui.m_checkBox_channel2->isChecked())
        list.push_back(2);

    if(ui.m_checkBox_channel3->isChecked())
        list.push_back(3);

    if(ui.m_checkBox_channel4->isChecked())
        list.push_back(4);

    if(ui.m_checkBox_channel5->isChecked())
        list.push_back(5);

    if(ui.m_checkBox_channel6->isChecked())
        list.push_back(6);

    if(ui.m_checkBox_channel7->isChecked())
        list.push_back(7);

    if(ui.m_checkBox_channel8->isChecked())
        list.push_back(8);

    if(ui.m_checkBox_channel9->isChecked())
        list.push_back(9);

    if(ui.m_checkBox_channel10->isChecked())
        list.push_back(10);

    if(ui.m_checkBox_channel11->isChecked())
        list.push_back(11);

    if(ui.m_checkBox_channel12->isChecked())
        list.push_back(12);

    if(ui.m_checkBox_channel13->isChecked())
        list.push_back(13);

    if(ui.m_checkBox_channel14->isChecked())
        list.push_back(14);

    if(ui.m_checkBox_channel15->isChecked())
        list.push_back(15);

    if(ui.m_checkBox_channel16->isChecked())
        list.push_back(16);

    if(!list.empty())
    {
    //store the channels-to-acquire-list in the according member variable
    m_pGUSBAmp->m_viChannelsToAcquire.resize(list.size());
    m_pGUSBAmp->m_viChannelsToAcquire = list;
    }
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannelSelect(bool checked)
{
    if(!checked)
    {
        ui.m_checkBox_channel1->setChecked(true);
        ui.m_checkBox_channel2->setChecked(true);
        ui.m_checkBox_channel3->setChecked(true);
        ui.m_checkBox_channel4->setChecked(true);
        ui.m_checkBox_channel5->setChecked(true);
        ui.m_checkBox_channel6->setChecked(true);
        ui.m_checkBox_channel7->setChecked(true);
        ui.m_checkBox_channel8->setChecked(true);
        ui.m_checkBox_channel9->setChecked(true);
        ui.m_checkBox_channel10->setChecked(true);
        ui.m_checkBox_channel11->setChecked(true);
        ui.m_checkBox_channel12->setChecked(true);
        ui.m_checkBox_channel13->setChecked(true);
        ui.m_checkBox_channel14->setChecked(true);
        ui.m_checkBox_channel15->setChecked(true);
        ui.m_checkBox_channel16->setChecked(true);

        checkBoxes();
    }

    if(checked)
    {
        ui.m_checkBox_channel1->setChecked(false);
        ui.m_checkBox_channel2->setChecked(false);
        ui.m_checkBox_channel3->setChecked(false);
        ui.m_checkBox_channel4->setChecked(false);
        ui.m_checkBox_channel5->setChecked(false);
        ui.m_checkBox_channel6->setChecked(false);
        ui.m_checkBox_channel7->setChecked(false);
        ui.m_checkBox_channel8->setChecked(false);
        ui.m_checkBox_channel9->setChecked(false);
        ui.m_checkBox_channel10->setChecked(false);
        ui.m_checkBox_channel11->setChecked(false);
        ui.m_checkBox_channel12->setChecked(false);
        ui.m_checkBox_channel13->setChecked(false);
        ui.m_checkBox_channel14->setChecked(false);
        ui.m_checkBox_channel15->setChecked(false);
        ui.m_checkBox_channel16->setChecked(false);

        checkBoxes();
    }
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel1()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel2()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel3()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel4()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel5()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel6()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel7()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel8()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel9()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel10()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel11()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel12()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel13()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel14()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel15()
{
    checkBoxes();
}

//=============================================================================================================

void GUSBAmpSetupWidget::activateChannel16()
{
    checkBoxes();
}
