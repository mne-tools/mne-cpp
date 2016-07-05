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
    QDialog(parent)
  ,  m_pSSVEPBCI(pssvepBCI)
  ,  ui(new Ui::ssvepBCIConfigurationWidget)
  ,  m_bInit(true)
{
    ui->setupUi(this);

    // default threshold values
    m_lSSVEPThresholdValues << 0.15 << 0.146 << 0.155 << 0.15;

    // set default threshold values
    ui->m_DoubleSpinBox_Threshold1->setValue(m_lSSVEPThresholdValues.at(0));
    ui->m_DoubleSpinBox_Threshold2->setValue(m_lSSVEPThresholdValues.at(1));
    ui->m_DoubleSpinBox_Threshold3->setValue(m_lSSVEPThresholdValues.at(2));
    ui->m_DoubleSpinBox_Threshold4->setValue(m_lSSVEPThresholdValues.at(3));
    //    ui->m_DoubleSpinBox_Threshold5->setValue(m_lSSVEPThresholdValues[4]);

    // edit Style sheets of the QProgressBars of threshold values (no blinking animation and pointy slider handle for threshold vixualization)
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

    // connect signals for power line elimination
    connect(ui->m_GroupBox_RemovePowerLine, &QGroupBox::toggled, m_pSSVEPBCI, &ssvepBCI::removePowerLine);
    connect(ui->m_SpinBox_PowerLineFrequency, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged) , m_pSSVEPBCI, &ssvepBCI::setPowerLine);

    // connect feature extraction behaviour signal
    connect(ui->m_RadioButton_MEC, &QRadioButton::toggled, m_pSSVEPBCI, &ssvepBCI::setFeatureExtractionMethod);

    // connect number of harmonicssignal
    connect(ui->m_SpinBox_NumOfHarmonics, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::numOfHarmonicsChanged);
    connect(this, &ssvepBCIConfigurationWidget::changeSSVEPParameter, m_pSSVEPBCI, &ssvepBCI::setChangeSSVEPParameterFlag);

    // connect SSVEP frequency List signal
    connect(m_pSSVEPBCI, &ssvepBCI::getFrequencyList, this, &ssvepBCIConfigurationWidget::setFrequencyList);

    // connect SSVEP values signal to refresh SSVEPProbabilities
    connect(m_pSSVEPBCI, &ssvepBCI::SSVEPprob, this, &ssvepBCIConfigurationWidget::setSSVEPProbabilities);
    qRegisterMetaType<MyQList>("MyQList");

    // connect changed threshold values
    connect(ui->m_DoubleSpinBox_Threshold1, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold2, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold3, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold4, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);

    // connect threshold values to BCI plugin
    connect(this, &ssvepBCIConfigurationWidget::getThresholdValues, m_pSSVEPBCI, &ssvepBCI::setThresholdValues);

    // connect classifiaction result
    connect(m_pSSVEPBCI, &ssvepBCI::classificationResult, ui->m_LcdNumber_ClasResult, static_cast<void(QLCDNumber::*)(double)>(&QLCDNumber::display));
    connect(m_pSSVEPBCI, &ssvepBCI::classificationResult, this, &ssvepBCIConfigurationWidget::setClassResult);

    // set palette for text color change
    m_palBlackFont.setColor(QPalette::WindowText, Qt::black);
    m_palRedFont.setColor(QPalette::WindowText, Qt::red);

    // initialize values to GUI-surface
    setFrequencyList(m_pSSVEPBCI->getCurrentListOfFrequencies());
    thresholdChanged(0);
    initSelectedChannelsSensor();
}


//*************************************************************************************************************

ssvepBCIConfigurationWidget::~ssvepBCIConfigurationWidget()
{
    delete ui;
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::setSSVEPProbabilities(MyQList SSVEP){

    // determine scale for threshold status bar
    if(m_bInit){
        m_dMaxProbValue = *std::max_element(SSVEP.begin(), SSVEP.end());
        m_dMinProbValue = *std::min_element(SSVEP.begin(), SSVEP.end());
        m_bInit = !m_bInit;
    }
    else{
        double min = *std::min_element(SSVEP.begin(), SSVEP.end());
        double max = *std::max_element(SSVEP.begin(), SSVEP.end());
        m_dMinProbValue = min < m_dMinProbValue ? min : m_dMinProbValue;
        m_dMaxProbValue = max > m_dMaxProbValue ? max : m_dMaxProbValue;
    }

    // scale SSVEP values for status bar
    QList<int> values;
    for(int i = 0; i < SSVEP.size(); i++)
        values << int((SSVEP.at(i) - m_dMinProbValue) / ( m_dMaxProbValue - m_dMinProbValue ) * 100);

    // assign SSVEP values to the status bar
    ui->m_ProgressBar_Threshold1->setValue(values[0]);
    ui->m_ProgressBar_Threshold2->setValue(values[1]);
    ui->m_ProgressBar_Threshold3->setValue(values[2]);
    ui->m_ProgressBar_Threshold4->setValue(values[3]);

    // assign SSVEP values to the labels
    ui->m_Label_SSVEP1->setText(QString::number(SSVEP[0]));
    ui->m_Label_SSVEP2->setText(QString::number(SSVEP[1]));
    ui->m_Label_SSVEP3->setText(QString::number(SSVEP[2]));
    ui->m_Label_SSVEP4->setText(QString::number(SSVEP[3]));

    // paint novel thresholds to screen
    updateThresholdsToScreen();

    // schedules an repaint event for the whole Threshold group box and all their childs
    ui->m_GroupBox_Threshold->update();
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


//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCIConfigurationWidget::on_m_RadioButton_MEC_toggled(bool checked)
{
    Q_UNUSED(checked);
    m_bInit = true;
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::thresholdChanged(double threshold)
{
    Q_UNUSED(threshold);

    // save threshold values to member varaible
    m_lSSVEPThresholdValues[0] = ui->m_DoubleSpinBox_Threshold1->value();
    m_lSSVEPThresholdValues[1] = ui->m_DoubleSpinBox_Threshold2->value();
    m_lSSVEPThresholdValues[2] = ui->m_DoubleSpinBox_Threshold3->value();
    m_lSSVEPThresholdValues[3] = ui->m_DoubleSpinBox_Threshold4->value();

    updateThresholdsToScreen();

    // emit thresholdValueChanged signal
    emit getThresholdValues(m_lSSVEPThresholdValues);
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::updateThresholdsToScreen(){

    // scale threshold values for slider
    QList<int> thresholds;
    for(int i = 0; i < m_lSSVEPThresholdValues.size(); i++)
        thresholds << int((m_lSSVEPThresholdValues.at(i) - m_dMinProbValue) / ( m_dMaxProbValue - m_dMinProbValue ) * 100);

    // assign SSVEP thresholds to sliders
    ui->m_VerticalSlider_Threshold1->setValue(thresholds[0]);
    ui->m_VerticalSlider_Threshold2->setValue(thresholds[1]);
    ui->m_VerticalSlider_Threshold3->setValue(thresholds[2]);
    ui->m_VerticalSlider_Threshold4->setValue(thresholds[3]);
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::setFrequencyList(MyQList frequencyList){

    // update frequency list
    m_lFrequencyList = frequencyList;

    // set labels
    ui->m_Label_Frequency1->setText(QString::number(m_lFrequencyList[0]).append(" Hz"));
    ui->m_Label_Frequency2->setText(QString::number(m_lFrequencyList[1]).append(" Hz"));
    ui->m_Label_Frequency3->setText(QString::number(m_lFrequencyList[2]).append(" Hz"));
    ui->m_Label_Frequency4->setText(QString::number(m_lFrequencyList[3]).append(" Hz"));

}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::setClassResult(double classResult){

    // set all labels to black again
    ui->m_Label_Frequency1->setPalette(m_palBlackFont);
    ui->m_Label_Frequency2->setPalette(m_palBlackFont);
    ui->m_Label_Frequency3->setPalette(m_palBlackFont);
    ui->m_Label_Frequency4->setPalette(m_palBlackFont);
    ui->m_Label_Frequency5->setPalette(m_palBlackFont);

    // highlighting the labels according to classifiaction result
    int index = m_lFrequencyList.indexOf(classResult);
    switch(index){
    case 0:
        ui->m_Label_Frequency1->setPalette(m_palRedFont); break;
    case 1:
        ui->m_Label_Frequency2->setPalette(m_palRedFont); break;
    case 2:
        ui->m_Label_Frequency3->setPalette(m_palRedFont); break;
    case 3:
        ui->m_Label_Frequency4->setPalette(m_palRedFont); break;
    case 4:
        ui->m_Label_Frequency5->setPalette(m_palRedFont); break;
    default:
        break;
    }
}


//*************************************************************************************************************

int ssvepBCIConfigurationWidget::getNumOfHarmonics(){
    return ui->m_SpinBox_NumOfHarmonics->value();
}



void ssvepBCIConfigurationWidget::numOfHarmonicsChanged(int harmonics){
    Q_UNUSED(harmonics);

    // emit signal for changing SSVEP Parameter
    emit changeSSVEPParameter();
}
