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

ssvepBCIConfigurationWidget::ssvepBCIConfigurationWidget(ssvepBCI* pssvepBCI, QWidget *parent)
: QDialog(parent)
, m_pSSVEPBCI(pssvepBCI)
, ui(new Ui::ssvepBCIConfigurationWidget)
, m_bInitThresholdDisplay(true)
, m_dMaxProbValue(1)
, m_dMinProbValue(0)
, m_bScreenKeyboardConnected(false)
, m_iWrongCommands(0)
, m_iCorrectCommands(0)
, m_iElapsedSeconds(0)
, m_qTimer(new QTimer)
{
    ui->setupUi(this);

    // default threshold values
    m_lSSVEPThresholdValues << 0.12 << 0.12 << 0.12 << 0.12 << 0.12;

    // set default threshold values
    ui->m_DoubleSpinBox_Threshold1->setValue(m_lSSVEPThresholdValues.at(0));
    ui->m_DoubleSpinBox_Threshold2->setValue(m_lSSVEPThresholdValues.at(1));
    ui->m_DoubleSpinBox_Threshold3->setValue(m_lSSVEPThresholdValues.at(2));
    ui->m_DoubleSpinBox_Threshold4->setValue(m_lSSVEPThresholdValues.at(3));
    ui->m_DoubleSpinBox_Threshold5->setValue(m_lSSVEPThresholdValues.at(4));

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

    // Connect channel selection
    connect(ui->m_listWidget_ChosenChannelsOnSensorLevel->model(), &QAbstractItemModel::rowsInserted, this, &ssvepBCIConfigurationWidget::channelSelectChanged);
    connect(ui->m_listWidget_ChosenChannelsOnSensorLevel->model(), &QAbstractItemModel::rowsRemoved, this, &ssvepBCIConfigurationWidget::channelSelectChanged);

    // connect signals for power line elimination
    connect(ui->m_GroupBox_RemovePowerLine, &QGroupBox::toggled, m_pSSVEPBCI, &ssvepBCI::removePowerLine);
    connect(ui->m_SpinBox_PowerLineFrequency, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged) , m_pSSVEPBCI, &ssvepBCI::setPowerLine);

    // connect feature extraction behaviour signal
    connect(ui->m_RadioButton_MEC, &QRadioButton::toggled, m_pSSVEPBCI, &ssvepBCI::setFeatureExtractionMethod);

    // connect number of harmonicssignal
    connect(ui->m_SpinBox_NumOfHarmonics, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::numOfHarmonicsChanged);
    connect(this, &ssvepBCIConfigurationWidget::changeSSVEPParameter, m_pSSVEPBCI, &ssvepBCI::setChangeSSVEPParameterFlag);

    // connect SSVEP frequency List signal
    connect(m_pSSVEPBCI, &ssvepBCI::getFrequencyLabels, this, &ssvepBCIConfigurationWidget::setFrequencyLabels);

    // connect SSVEP values signal to refresh SSVEPProbabilities
    connect(m_pSSVEPBCI, &ssvepBCI::SSVEPprob, this, &ssvepBCIConfigurationWidget::setSSVEPProbabilities);
    qRegisterMetaType<MyQList>("MyQList");

    // connect changed threshold values
    connect(ui->m_DoubleSpinBox_Threshold1, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold2, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold3, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold4, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);
    connect(ui->m_DoubleSpinBox_Threshold5, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ssvepBCIConfigurationWidget::thresholdChanged);

    // connect threshold values to BCI plugin
    connect(this, &ssvepBCIConfigurationWidget::getThresholdValues, m_pSSVEPBCI, &ssvepBCI::setThresholdValues);

    // connect classifiaction result to LCD-Display and colour change for labels
    connect(m_pSSVEPBCI, &ssvepBCI::classificationResult, ui->m_LcdNumber_ClasResult, static_cast<void(QLCDNumber::*)(double)>(&QLCDNumber::display));
    connect(m_pSSVEPBCI, &ssvepBCI::classificationResult, this, &ssvepBCIConfigurationWidget::setClassResult);

    // connect classifiaction hits-spinbox and classification breaks-spinbox
    connect(ui->m_spinBox_ClassificationHits, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_pSSVEPBCI, &ssvepBCI::setNumClassHits);
    connect(ui->m_spinBox_ClassificationBreaks, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_pSSVEPBCI, &ssvepBCI::setNumClassBreaks);
    connect(ui->m_spinBox_ClassificationListSize, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_pSSVEPBCI, &ssvepBCI::setSizeClassList);

    // connect timer to display elapsed time in widget
    connect(m_qTimer, &QTimer::timeout, this, &ssvepBCIConfigurationWidget::showCurrentTime);

    // set palette for text color change
    m_palBlackFont.setColor(QPalette::WindowText, Qt::black);
    m_palRedFont.setColor(QPalette::WindowText, Qt::red);

    // initialize values to GUI-surface
    setFrequencyLabels(m_pSSVEPBCI->getCurrentListOfFrequencies());
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
    if(m_bInitThresholdDisplay){
        // reset scale for threshold values & status bar
        m_dMaxProbValue = *std::max_element(SSVEP.begin(), SSVEP.end());
        m_dMinProbValue = *std::min_element(SSVEP.begin(), SSVEP.end());
        resetThresholdValues();
        m_bInitThresholdDisplay = false;
    }
    else{
        // update scale for threshold status bar
        double min = *std::min_element(SSVEP.begin(), SSVEP.end());
        double max = *std::max_element(SSVEP.begin(), SSVEP.end());
        m_dMinProbValue = min < m_dMinProbValue ? min : m_dMinProbValue;
        m_dMaxProbValue = max > m_dMaxProbValue ? max : m_dMaxProbValue;
    }

    // filling rest of SSVEP with zeros if size of SSVEP < 5
    for(int i = SSVEP.size(); i < 5; i++)
        SSVEP << m_dMinProbValue;

    // scale SSVEP values for status bar
    QList<int> values;
    for(int i = 0; i < SSVEP.size(); i++)
        values << int((SSVEP.at(i) - m_dMinProbValue) / ( m_dMaxProbValue - m_dMinProbValue ) * 100);

    // assign SSVEP values to the status bar
    ui->m_ProgressBar_Threshold1->setValue(values[0]);
    ui->m_ProgressBar_Threshold2->setValue(values[1]);
    ui->m_ProgressBar_Threshold3->setValue(values[2]);
    ui->m_ProgressBar_Threshold4->setValue(values[3]);
    ui->m_ProgressBar_Threshold5->setValue(values[4]);

    // assign SSVEP values to the labels
    ui->m_Label_SSVEP1->setText(QString::number(SSVEP[0]));
    ui->m_Label_SSVEP2->setText(QString::number(SSVEP[1]));
    ui->m_Label_SSVEP3->setText(QString::number(SSVEP[2]));
    ui->m_Label_SSVEP4->setText(QString::number(SSVEP[3]));
    ui->m_Label_SSVEP5->setText(QString::number(SSVEP[4]));

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
    for(int i=0; i<m_pSSVEPBCI->m_slChosenChannelsSensor.size(); i++)
        m_vAvailableChannelsSensor.removeAt(m_vAvailableChannelsSensor.indexOf(m_pSSVEPBCI->m_slChosenChannelsSensor.at(i)));

    ui->m_listWidget_AvailableChannelsOnSensorLevel->addItems(m_vAvailableChannelsSensor);
    ui->m_listWidget_ChosenChannelsOnSensorLevel->addItems(m_pSSVEPBCI->m_slChosenChannelsSensor);
}


//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCIConfigurationWidget::on_m_RadioButton_MEC_toggled(bool checked)
{
    Q_UNUSED(checked);
    m_bInitThresholdDisplay = true;
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
    m_lSSVEPThresholdValues[4] = ui->m_DoubleSpinBox_Threshold5->value();

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
    ui->m_VerticalSlider_Threshold5->setValue(thresholds[4]);
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::setFrequencyLabels(MyQList frequencyList){

    // filling the list with missing zeros
    for(int i = frequencyList.size(); i < 6; i++)
        frequencyList << 0;

    // update frequency list
    m_lFrequencyList = frequencyList;

    // set labels
    ui->m_Label_Frequency1->setText(QString::number(m_lFrequencyList[0]).append(" Hz"));
    ui->m_Label_Frequency2->setText(QString::number(m_lFrequencyList[1]).append(" Hz"));
    ui->m_Label_Frequency3->setText(QString::number(m_lFrequencyList[2]).append(" Hz"));
    ui->m_Label_Frequency4->setText(QString::number(m_lFrequencyList[3]).append(" Hz"));
    ui->m_Label_Frequency5->setText(QString::number(m_lFrequencyList[4]).append(" Hz"));

    // reset borders of status bar
    m_bInitThresholdDisplay = true;

}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::setClassResult(double classResult){

    // set all labels to black again
    ui->m_Label_Frequency1->setPalette(m_palBlackFont);
    ui->m_Label_Frequency2->setPalette(m_palBlackFont);
    ui->m_Label_Frequency3->setPalette(m_palBlackFont);
    ui->m_Label_Frequency4->setPalette(m_palBlackFont);
    ui->m_Label_Frequency5->setPalette(m_palBlackFont);

    // highlighting the labels according to classifiaction result as long the result is not equal 0
    if(classResult != 0){
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
}


//*************************************************************************************************************

int ssvepBCIConfigurationWidget::getNumOfHarmonics(){
    return ui->m_SpinBox_NumOfHarmonics->value();
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::numOfHarmonicsChanged(int harmonics){

    Q_UNUSED(harmonics);

    // reset scale of the threshold status bar
    m_bInitThresholdDisplay = true;

    // emit signal for changing SSVEP Parameter
    emit changeSSVEPParameter();
}


//*************************************************************************************************************

QStringList ssvepBCIConfigurationWidget::getSensorChannelSelection(){

    QStringList ChosenSensorChannelSelect;
    for(int i=0; i< ui->m_listWidget_ChosenChannelsOnSensorLevel->count(); i++)
        ChosenSensorChannelSelect << ui->m_listWidget_ChosenChannelsOnSensorLevel->item(i)->text();

    return ChosenSensorChannelSelect;
}


//*************************************************************************************************************

QStringList ssvepBCIConfigurationWidget::getSourceChannelSelection(){

    QStringList ChosenSourceChannelSelect;
    for(int i=0; i< ui->m_listWidget_ChosenChannelsOnSourceLevel->count(); i++)
        ChosenSourceChannelSelect << ui->m_listWidget_ChosenChannelsOnSourceLevel->item(i)->text();

    return ChosenSourceChannelSelect;
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::channelSelectChanged(const QModelIndex &parent, int first, int last){

    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    // reset threshold display
    m_bInitThresholdDisplay = true;

    // emit signal for changing ssvep parameter after BCI processing
    emit changeSSVEPParameter();
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::resetThresholdValues(){

    // assign SSVEP thresholds to sliders
    ui->m_DoubleSpinBox_Threshold1->setValue(m_dMaxProbValue);
    ui->m_DoubleSpinBox_Threshold2->setValue(m_dMaxProbValue);
    ui->m_DoubleSpinBox_Threshold3->setValue(m_dMaxProbValue);
    ui->m_DoubleSpinBox_Threshold4->setValue(m_dMaxProbValue);
    ui->m_DoubleSpinBox_Threshold5->setValue(m_dMaxProbValue);
}


//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCIConfigurationWidget::on_m_pushButton_StartMeasurement_clicked()
{
    if(m_pSSVEPBCI->m_pssvepBCISetupStimulusWidget != NULL){
        QSharedPointer<ScreenKeyboard> pScreenKeyboard = m_pSSVEPBCI->m_pssvepBCISetupStimulusWidget->getScreenKeyboardSPtr();
        if(pScreenKeyboard != NULL){
            pScreenKeyboard->initSpellAccuracyFeature();

            if(!m_bScreenKeyboardConnected){
                connect(pScreenKeyboard.data(), &ScreenKeyboard::isCorrectCommand, this, &ssvepBCIConfigurationWidget::evaluateCommand);
                connect(pScreenKeyboard.data(), &ScreenKeyboard::spellingFinished, this, &ssvepBCIConfigurationWidget::stopMeasurement);

                m_bScreenKeyboardConnected = true;
            }

        }
    }

    m_qTimer->start(1000);
    m_iElapsedSeconds = 0;

    // reset counter for command evaluation
    m_iWrongCommands = 0;
    m_iCorrectCommands = 0;

    ui->m_label_CorrectCommands->setText(QString::number(m_iCorrectCommands));
    ui->m_label_WrongCommands->setText(QString::number(m_iWrongCommands));
}


//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCIConfigurationWidget::on_m_pushButton_StopMeasurement_clicked()
{
    if(m_pSSVEPBCI->m_pssvepBCISetupStimulusWidget != NULL){
        QSharedPointer<ScreenKeyboard> pScreenKeyboard = m_pSSVEPBCI->m_pssvepBCISetupStimulusWidget->getScreenKeyboardSPtr();
        if(pScreenKeyboard != NULL)
            pScreenKeyboard->stopSpellAccuracyFeature();

    }


    stopMeasurement();
}


//*************************************************************************************************************

void ssvepBCIConfigurationWidget::evaluateCommand(bool isCorrectCommand){

    if(isCorrectCommand){
        m_iCorrectCommands += 1;
        ui->m_label_CorrectCommands->setText(QString::number(m_iCorrectCommands));
    }
    else{
        m_iWrongCommands +=1;
        ui->m_label_WrongCommands->setText(QString::number(m_iWrongCommands));
    }

}


//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCIConfigurationWidget::on_m_spinBox_ClassificationListSize_valueChanged(int arg1)
{
    ui->m_spinBox_ClassificationHits->setMaximum(arg1);
}


void ssvepBCIConfigurationWidget::showCurrentTime(){

    m_iElapsedSeconds++;
    QString time = QString::number(m_iElapsedSeconds);
    ui->m_label_ElapsedTime->setText(time);
}

void ssvepBCIConfigurationWidget::stopMeasurement(){

    m_qTimer->stop();
}
