//=============================================================================================================
/**
* @file     ssvepbciconfigurationwidget.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenauz.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June 2016
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the ssvepBCIConfigurationWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbciconfigurationwidget.h"
#include "ui_ssvepbciconfigurationwidget.h"
#include "../ssvepbci.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ssvepBCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ssvepBCIConfigurationWidget::ssvepBCIConfigurationWidget(ssvepBCI* pssvepBCI, QWidget *parent) :
    QDialog(parent),
    m_pSSVEPBCI(pssvepBCI),
    ui(new Ui::ssvepBCIConfigurationWidget)
{
    ui->setupUi(this);

    //set Style sheet of the QProgressBar (no blinking animation and pointy slider handle)
    ui->m_ProgressBar_Threshold1->setStyleSheet(" QProgressBar { border: 2px solid grey; border-radius: 2px; } QProgressBar::chunk {background-color: #3add36;}");
    ui->m_ProgressBar_Threshold1->setTextVisible(false);
    ui->m_VerticalSlider_Threshold1->setStyleSheet("QSlider::handle {image: url(:/images/slider_handle.png);}");
    ui->m_ProgressBar_Threshold2->setStyleSheet(" QProgressBar { border: 2px solid grey; border-radius: 2px; } QProgressBar::chunk {background-color: #3add36;}");
    ui->m_ProgressBar_Threshold2->setTextVisible(false);
    ui->m_VerticalSlider_Threshold2->setStyleSheet("QSlider::handle {image: url(:/images/slider_handle.png);}");
    ui->m_ProgressBar_Threshold3->setStyleSheet(" QProgressBar { border: 2px solid grey; border-radius: 2px; } QProgressBar::chunk {background-color: #3add36;}");
    ui->m_ProgressBar_Threshold3->setTextVisible(false);
    ui->m_VerticalSlider_Threshold3->setStyleSheet("QSlider::handle {image: url(:/images/slider_handle.png);}");
    ui->m_ProgressBar_Threshold4->setStyleSheet(" QProgressBar { border: 2px solid grey; border-radius: 2px; } QProgressBar::chunk {background-color: #3add36;}");
    ui->m_ProgressBar_Threshold4->setTextVisible(false);
    ui->m_VerticalSlider_Threshold4->setStyleSheet("QSlider::handle {image: url(:/images/slider_handle.png);}");
    ui->m_ProgressBar_Threshold5->setStyleSheet(" QProgressBar { border: 2px solid grey; border-radius: 2px; } QProgressBar::chunk {background-color: #3add36;}");
    ui->m_ProgressBar_Threshold5->setTextVisible(false);
    ui->m_VerticalSlider_Threshold5->setStyleSheet("QSlider::handle {image: url(:/images/slider_handle.png);}");



    initSelectedChannelsSensor();
}

//*************************************************************************************************************

ssvepBCIConfigurationWidget::~ssvepBCIConfigurationWidget()
{
    delete ui;
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}

//*************************************************************************************************************

void ssvepBCIConfigurationWidget::initSelectedChannelsSensor()
{
    // Read electrode pinnig scheme from file and initialise List and store in QMap in BCI object
    QString path;
    path.prepend(m_pSSVEPBCI->m_qStringResourcePath);
    path.append("Pinning_Scheme_Duke_Dry_64.txt");
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    //Start reading from file
    m_vAvailableChannelsSensor.clear();
    QMap<QString, int>  mapElectrodePinningScheme;

    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line = in.readLine();
        QStringList list_temp = line.split(QRegExp("\\s+"));

        if(list_temp.size() >= 2)
            mapElectrodePinningScheme.insert(list_temp.at(1), list_temp.at(0).toInt()-1); // Decrement by 1 because channels in matrix start with 0
        m_vAvailableChannelsSensor.append(list_temp.at(1));
    }
    file.close();
    m_pSSVEPBCI->m_mapElectrodePinningScheme = mapElectrodePinningScheme;

    // Remove default items from list
    for(int i=0; i<m_pSSVEPBCI->m_slChosenFeatureSensor.size(); i++)
        m_vAvailableChannelsSensor.removeAt(m_vAvailableChannelsSensor.indexOf(m_pSSVEPBCI->m_slChosenFeatureSensor.at(i)));

    ui->m_listWidget_AvailableChannelsOnSensorLevel->addItems(m_vAvailableChannelsSensor);
    ui->m_listWidget_ChosenChannelsOnSensorLevel->addItems(m_pSSVEPBCI->m_slChosenFeatureSensor);
}
