//=============================================================================================================
/**
* @file     scalewindow.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2014
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
* @brief    Definition of the ScaleWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scalewindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScaleWindow::ScaleWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ScaleWindow)
{
    ui->setupUi(this);
}


//*************************************************************************************************************

ScaleWindow::~ScaleWindow()
{
    delete ui;
}


//*************************************************************************************************************

void ScaleWindow::init()
{
    //Connect data scaling spin boxes
    connect(ui->m_doubleSpinBox_MEG_grad,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_MEG_mag,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_EEG,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_EOG,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_EMG,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_ECG,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_MISC,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);
    connect(ui->m_doubleSpinBox_STIM,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,&ScaleWindow::scaleChannelValueChanged);

    //Connect view scaling spin boxes
    connect(ui->m_SpinBox_channelHeight,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,&ScaleWindow::scaleViewValueChanged);
}


//*************************************************************************************************************

QMap<QString,double> ScaleWindow::genereateScalingMap()
{
    QMap<QString,double> scaleMap;

    scaleMap["MEG_grad"] = ui->m_doubleSpinBox_MEG_grad->value() * 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
    scaleMap["MEG_mag"] = ui->m_doubleSpinBox_MEG_mag->value() * 1e-12;
    scaleMap["MEG_EEG"] = ui->m_doubleSpinBox_EEG->value() * 1e-06;
    scaleMap["MEG_EOG"] = ui->m_doubleSpinBox_EOG->value() * 1e-06;
    scaleMap["MEG_EMG"] = ui->m_doubleSpinBox_EMG->value() * 1e-03;
    scaleMap["MEG_ECG"] = ui->m_doubleSpinBox_ECG->value() * 1e-03;
    scaleMap["MEG_MISC"] = ui->m_doubleSpinBox_MISC->value() * 1;
    scaleMap["MEG_STIM"] = ui->m_doubleSpinBox_STIM->value() * 1;

    return scaleMap;
}


//*************************************************************************************************************

void ScaleWindow::hideSpinBoxes(FiffInfo::SPtr& pCurrentFiffInfo)
{
    //Hide all spin boxes and labels
    ui->m_doubleSpinBox_MEG_grad->hide();
    ui->m_doubleSpinBox_MEG_mag->hide();
    ui->m_doubleSpinBox_EEG->hide();
    ui->m_doubleSpinBox_EOG->hide();
    ui->m_doubleSpinBox_EMG->hide();
    ui->m_doubleSpinBox_ECG->hide();
    ui->m_doubleSpinBox_MISC->hide();
    ui->m_doubleSpinBox_STIM->hide();

    ui->m_label_MEG_grad->hide();
    ui->m_label_MEG_mag->hide();
    ui->m_label_EEG->hide();
    ui->m_label_EOG->hide();
    ui->m_label_EMG->hide();
    ui->m_label_ECG->hide();
    ui->m_label_MISC->hide();
    ui->m_label_STIM->hide();

    //Show only spin boxes and labels which type are present in the current loaded fiffinfo
    QList<FiffChInfo> channelList = pCurrentFiffInfo->chs;
    for(int i = 0; i<channelList.size(); i++) {
        switch(channelList.at(i).kind) {
        case FIFFV_MEG_CH: {
            qint32 unit = channelList.at(i).unit;
            if(unit == FIFF_UNIT_T_M) {
                //Gradiometers
                ui->m_doubleSpinBox_MEG_grad->show();
                ui->m_label_MEG_grad->show();
            }
            else if(unit == FIFF_UNIT_T) {
                //Magnitometers
                ui->m_doubleSpinBox_MEG_mag->show();
                ui->m_label_MEG_mag->show();
            }

            break;
        }

        case FIFFV_EEG_CH: {
            ui->m_label_EEG->show();
            ui->m_doubleSpinBox_EEG->show();
            break;
        }

        case FIFFV_EOG_CH: {
            ui->m_label_EOG->show();
            ui->m_doubleSpinBox_EOG->show();
            break;
        }

        case FIFFV_EMG_CH: {
            ui->m_label_EMG->show();
            ui->m_doubleSpinBox_EMG->show();
            break;
        }

        case FIFFV_ECG_CH: {
            ui->m_label_ECG->show();
            ui->m_doubleSpinBox_ECG->show();
            break;
        }

        case FIFFV_MISC_CH: {
            ui->m_label_MISC->show();
            ui->m_doubleSpinBox_MISC->show();
            break;
        }

        case FIFFV_STIM_CH: {
            ui->m_label_STIM->show();
            ui->m_doubleSpinBox_STIM->show();
            break;
        }

        }
    }
}


//*************************************************************************************************************

void ScaleWindow::scaleAllChannels(double scaleValue)
{
    qDebug()<<scaleValue;

    scaleValue = (scaleValue - 1)*-4;

    ui->m_doubleSpinBox_MEG_grad->setValue((scaleValue*ui->m_doubleSpinBox_MEG_grad->singleStep()) + ui->m_doubleSpinBox_MEG_grad->value());
    ui->m_doubleSpinBox_MEG_mag->setValue((scaleValue*ui->m_doubleSpinBox_MEG_mag->singleStep()) + ui->m_doubleSpinBox_MEG_mag->value());
    ui->m_doubleSpinBox_EEG->setValue((scaleValue*ui->m_doubleSpinBox_EEG->singleStep()) + ui->m_doubleSpinBox_EEG->value());
    ui->m_doubleSpinBox_EOG->setValue((scaleValue*ui->m_doubleSpinBox_EOG->singleStep()) + ui->m_doubleSpinBox_EOG->value());
    ui->m_doubleSpinBox_EMG->setValue((scaleValue*ui->m_doubleSpinBox_EMG->singleStep()) + ui->m_doubleSpinBox_EMG->value());
    ui->m_doubleSpinBox_ECG->setValue((scaleValue*ui->m_doubleSpinBox_ECG->singleStep()) + ui->m_doubleSpinBox_ECG->value());
    ui->m_doubleSpinBox_MISC->setValue((scaleValue*ui->m_doubleSpinBox_MISC->singleStep()) + ui->m_doubleSpinBox_MISC->value());
    ui->m_doubleSpinBox_STIM->setValue((scaleValue*ui->m_doubleSpinBox_STIM->singleStep()) + ui->m_doubleSpinBox_STIM->value());
}


//*************************************************************************************************************

void ScaleWindow::scaleChannelValueChanged()
{
    emit scalingChannelValueChanged(genereateScalingMap());
}


//*************************************************************************************************************

void ScaleWindow::scaleViewValueChanged()
{
    emit scalingViewValueChanged(ui->m_SpinBox_channelHeight->value());
}
