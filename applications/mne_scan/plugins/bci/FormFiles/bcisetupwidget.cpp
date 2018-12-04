//=============================================================================================================
/**
* @file     bcisetupwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*			Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BCISetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bcisetupwidget.h"
#include "bciaboutwidget.h"
#include "../bci.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BCIPLUGIN;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BCISetupWidget::BCISetupWidget(BCI* pBCI, QWidget* parent)
: QWidget(parent)
, m_pBCI(pBCI)
{
    ui.setupUi(this);

    // Connect general options
    connect(ui.m_checkBox_UseSourceData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_checkBox_UseSensorData, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_checkBox_DisplayFeatures, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_SpinBox_NumberFeaturesToDisplay, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_doubleSpinBox_Boundary, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_doubleSpinBox_Variances, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_doubleSpinBox_Electrodes, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_spinBox_exponentBoundary, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_spinBox_exponentVariances, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);
    connect(ui.m_spinBox_exponentElectrodes, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setGeneralOptions);

    // Connect processing options
    connect(ui.m_checkBox_SubtractMean, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_doubleSpinBox_SlidingWindowSize, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_spinBox_NumberFeatures, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_doubleSpinBox_TimeBetweenWindows, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_checkBox_UseThresholdArtefactReduction, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setProcessingOptions);
    connect(ui.m_SpinBox_ThresholdValue, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setProcessingOptions);

    // Connect classification options
    connect(ui.m_comboBox_featureCalculationType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BCISetupWidget::setClassificationOptions);

    connect(ui.m_pushButton_LoadSensorBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSensorBoundary);
    connect(ui.m_pushButton_LoadSourceBoundary, &QPushButton::released,
            this, &BCISetupWidget::changeLoadSourceBoundary);

    // Connect feature selection
    ui.m_listWidget_ChosenFeaturesOnSensorLevel->installEventFilter(this);
    ui.m_listWidget_ChosenFeaturesOnSourceLevel->installEventFilter(this);
    ui.m_listWidget_AvailableFeaturesOnSensorLevel->installEventFilter(this);
    ui.m_listWidget_AvailableFeaturesOnSourceLevel->installEventFilter(this);

    // Connect filter options
    connect(ui.m_checkBox_UseFilter, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &BCISetupWidget::setFilterOptions);
    connect(ui.m_doubleSpinBox_FilterLowerBound, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setFilterOptions);
    connect(ui.m_doubleSpinBox_FilterUpperBound, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setFilterOptions);
    connect(ui.m_SpinBox_FilterOrder, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &BCISetupWidget::setFilterOptions);
    connect(ui.m_doubleSpinBox_ParcksWidth, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
            this, &BCISetupWidget::setFilterOptions);

    //Connect about button
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &BCISetupWidget::showAboutDialog);

    //Fill info box
    QFile file(m_pBCI->m_qStringResourcePath+"readme.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        ui.m_qTextBrowser_Information->insertHtml(line);
        ui.m_qTextBrowser_Information->insertHtml("<br>");
    }
}


//*************************************************************************************************************

BCISetupWidget::~BCISetupWidget()
{
}


//*************************************************************************************************************

void BCISetupWidget::initGui()
{
    // General options
    ui.m_checkBox_UseSensorData->setChecked(m_pBCI->m_bUseSensorData);
    ui.m_checkBox_UseSourceData->setChecked(m_pBCI->m_bUseSourceData);
    ui.m_checkBox_DisplayFeatures->setChecked(m_pBCI->m_bDisplayFeatures);
    ui.m_SpinBox_NumberFeaturesToDisplay->setValue(m_pBCI->m_iNumberFeaturesToDisplay);
    ui.m_doubleSpinBox_Boundary->setValue(m_pBCI->m_dDisplayRangeBoundary);
    ui.m_doubleSpinBox_Variances->setValue(m_pBCI->m_dDisplayRangeVariances);
    ui.m_doubleSpinBox_Electrodes->setValue(m_pBCI->m_dDisplayRangeElectrodes);

    setGeneralOptions(); // Set up ranges for the first time

    // Processing options
    ui.m_checkBox_SubtractMean->setChecked(m_pBCI->m_bSubtractMean);
    ui.m_doubleSpinBox_SlidingWindowSize->setValue(m_pBCI->m_dSlidingWindowSize);
    ui.m_doubleSpinBox_TimeBetweenWindows->setValue(m_pBCI->m_dTimeBetweenWindows);
    ui.m_spinBox_NumberFeatures->setValue(m_pBCI->m_iNumberFeatures);
    ui.m_checkBox_UseThresholdArtefactReduction->setChecked(m_pBCI->m_bUseArtefactThresholdReduction);
    ui.m_SpinBox_ThresholdValue->setValue(m_pBCI->m_dThresholdValue);

    // Init time displays
    double totalProcessingTime = ui.m_doubleSpinBox_SlidingWindowSize->value() + (ui.m_doubleSpinBox_TimeBetweenWindows->value() * (ui.m_spinBox_NumberFeatures->value()-1));
    ui.m_label_TotalProcessedTimeDisplay->setNum(totalProcessingTime);
    ui.m_label_TotalProcessedTimeDisplay->setText(ui.m_label_TotalProcessedTimeDisplay->text().append(" s"));

    double timeForNextResult = ui.m_doubleSpinBox_TimeBetweenWindows->value() * ui.m_spinBox_NumberFeatures->value();
    ui.m_label_TimeNeededForResultsDisplay->setNum(timeForNextResult);
    ui.m_label_TimeNeededForResultsDisplay->setText(ui.m_label_TimeNeededForResultsDisplay->text().append(" s"));

    // Classification boundaries
    ui.m_comboBox_featureCalculationType->setCurrentIndex(m_pBCI->m_iFeatureCalculationType);
    QString temp = m_pBCI->m_qStringResourcePath;
    temp.append(QString("LDA_linear_boundary_Sensor.txt"));
    ui.m_lineEdit_SensorBoundary->setText(temp);
    ui.m_lineEdit_SourceBoundary->setText(temp);
    m_pBCI->m_vLoadedSensorBoundary = readBoundaryInformation(temp);
    m_pBCI->m_vLoadedSourceBoundary = readBoundaryInformation(temp);

    // Filter options
    ui.m_checkBox_UseFilter->setChecked(m_pBCI->m_bUseFilter);
    ui.m_doubleSpinBox_FilterLowerBound->setValue(m_pBCI->m_dFilterLowerBound);
    ui.m_doubleSpinBox_FilterUpperBound->setValue(m_pBCI->m_dFilterUpperBound);
    ui.m_SpinBox_FilterOrder->setValue(m_pBCI->m_iFilterOrder);
    ui.m_doubleSpinBox_ParcksWidth->setValue(m_pBCI->m_dParcksWidth);

    // Selected features on sensor level
    initSelectedFeaturesSensor();
}


//*************************************************************************************************************

void BCISetupWidget::setGeneralOptions()
{
    m_pBCI->m_bUseSensorData = ui.m_checkBox_UseSensorData->isChecked();
    m_pBCI->m_bUseSourceData = ui.m_checkBox_UseSourceData->isChecked();
    m_pBCI->m_bDisplayFeatures = ui.m_checkBox_DisplayFeatures->isChecked();
    m_pBCI->m_iNumberFeaturesToDisplay = ui.m_SpinBox_NumberFeaturesToDisplay->value();

    m_pBCI->m_dDisplayRangeBoundary = ui.m_doubleSpinBox_Boundary->value() * std::pow(10, ui.m_spinBox_exponentBoundary->value());
    m_pBCI->m_dDisplayRangeVariances = ui.m_doubleSpinBox_Variances->value() * std::pow(10, ui.m_spinBox_exponentVariances->value());
    m_pBCI->m_dDisplayRangeElectrodes = ui.m_doubleSpinBox_Electrodes->value() * std::pow(10, ui.m_spinBox_exponentElectrodes->value());
}


//*************************************************************************************************************

void BCISetupWidget::setProcessingOptions()
{
    m_pBCI->m_bSubtractMean = ui.m_checkBox_SubtractMean->isChecked();
    m_pBCI->m_dSlidingWindowSize = ui.m_doubleSpinBox_SlidingWindowSize->value();
    m_pBCI->m_dTimeBetweenWindows = ui.m_doubleSpinBox_TimeBetweenWindows->value();    
    m_pBCI->m_iNumberFeatures = ui.m_spinBox_NumberFeatures->value();
    m_pBCI->m_bUseArtefactThresholdReduction = ui.m_checkBox_UseThresholdArtefactReduction->isChecked();
    m_pBCI->m_dThresholdValue = ui.m_SpinBox_ThresholdValue->value();

    double totalProcessingTime = ui.m_doubleSpinBox_SlidingWindowSize->value() + (ui.m_doubleSpinBox_TimeBetweenWindows->value() * (ui.m_spinBox_NumberFeatures->value()-1));
    ui.m_label_TotalProcessedTimeDisplay->setNum(totalProcessingTime);
    ui.m_label_TotalProcessedTimeDisplay->setText(ui.m_label_TotalProcessedTimeDisplay->text().append(" s"));

    double timeForNextResult = ui.m_doubleSpinBox_TimeBetweenWindows->value() * ui.m_spinBox_NumberFeatures->value();
    ui.m_label_TimeNeededForResultsDisplay->setNum(timeForNextResult);
    ui.m_label_TimeNeededForResultsDisplay->setText(ui.m_label_TimeNeededForResultsDisplay->text().append(" s"));
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSensorBoundary()
{
    QString path = QFileDialog::getOpenFileName(
                this,
                "Load decision boundary for sensor level",
                "resources/mne_scan_plugins/bci/LDA_linear_boundary.txt",
                 tr("Text files (*.txt)"));

    if(path==NULL)
        path = ui.m_lineEdit_SensorBoundary->text();

    m_pBCI->m_vLoadedSensorBoundary = readBoundaryInformation(path);

    ui.m_lineEdit_SensorBoundary->setText(path);
}


//*************************************************************************************************************

void BCISetupWidget::changeLoadSourceBoundary()
{
    QString path = QFileDialog::getOpenFileName(
                this,
                "Load decision boundary for source level",
                "resources/mne_scan_plugins/bci/LDA_linear_boundary.txt",
                 tr("Text files (*.txt)"));

    if(path==NULL)
        path = ui.m_lineEdit_SourceBoundary->text();


    m_pBCI->m_vLoadedSourceBoundary = readBoundaryInformation(path);

    ui.m_lineEdit_SourceBoundary->setText(path);
}


//*************************************************************************************************************

QVector<VectorXd> BCISetupWidget::readBoundaryInformation(QString path)
{
    QVector<VectorXd> boundary_final;

    // Read boundary information generated with matlab
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Initialise boundaries with linear coefficients y = mx+c -> vector = [m c] -> default [1 0]
        VectorXd const_temp(1);
        const_temp << 0.0;
        VectorXd linear_temp(2);
        linear_temp << 1.0, 1.0;

        boundary_final.push_back(const_temp);
        boundary_final.push_back(linear_temp);

        return boundary_final;
    }

    //Start reading from file
    VectorXd const_temp;
    VectorXd linear_temp;

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();

        if(line.contains(QString("const")))
        {
            QStringList list_temp = line.split(QRegExp("\\s+"));

            const_temp.resize(list_temp.at(1).toInt());

            for(int i = 0; i<list_temp.at(1).toInt(); i++)
            {
                QString line_temp = in.readLine();
                const_temp(i) = line_temp.toDouble();
            }
        }

        if(line.contains(QString("linear")))
        {
            QStringList list_temp = line.split(QRegExp("\\s+"));

            linear_temp.resize(list_temp.at(1).toInt());

            for(int i = 0; i<list_temp.at(1).toInt(); i++)
            {
                QString line_temp = in.readLine();
                linear_temp(i) = line_temp.toDouble();
            }
        }
    }

    file.close();

    boundary_final.push_back(const_temp);
    boundary_final.push_back(linear_temp);

    return boundary_final;
}


//*************************************************************************************************************

void BCISetupWidget::initSelectedFeaturesSensor()
{
    // Read electrode pinnig scheme from file and initialise List and store in QMap in BCI object
    QString path;
    path.prepend(m_pBCI->m_qStringResourcePath);
    path.append("Pinning_Scheme_Duke_128.txt");
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    //Start reading from file
    m_vAvailableFeaturesSensor.clear();
    QMap<QString, int>  mapElectrodePinningScheme;

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();

        QStringList list_temp = line.split(QRegExp("\\s+"));

        if(list_temp.size() >= 2)
            mapElectrodePinningScheme.insert(list_temp.at(1), list_temp.at(0).toInt()-1); // Decrement by 1 because channels in matrix start with 0

        m_vAvailableFeaturesSensor.append(list_temp.at(1));
    }

    file.close();

    m_pBCI->m_mapElectrodePinningScheme = mapElectrodePinningScheme;

    // Remove default items from list
    for(int i=0; i<m_pBCI->m_slChosenFeatureSensor.size(); i++)
        m_vAvailableFeaturesSensor.removeAt(m_vAvailableFeaturesSensor.indexOf(m_pBCI->m_slChosenFeatureSensor.at(i)));

    ui.m_listWidget_AvailableFeaturesOnSensorLevel->addItems(m_vAvailableFeaturesSensor);
    ui.m_listWidget_ChosenFeaturesOnSensorLevel->addItems(m_pBCI->m_slChosenFeatureSensor);
}


//*************************************************************************************************************

void BCISetupWidget::setFeatureSelection()
{
    QStringList ChosenFeaturesOnSensorLevel;
    for(int i=0; i< ui.m_listWidget_ChosenFeaturesOnSensorLevel->count(); i++)
        ChosenFeaturesOnSensorLevel << ui.m_listWidget_ChosenFeaturesOnSensorLevel->item(i)->text();

    QStringList ChosenFeaturesOnSourceLevel;
    for(int i=0; i< ui.m_listWidget_ChosenFeaturesOnSourceLevel->count(); i++)
        ChosenFeaturesOnSourceLevel << ui.m_listWidget_ChosenFeaturesOnSourceLevel->item(i)->text();

    m_pBCI->m_slChosenFeatureSensor = ChosenFeaturesOnSensorLevel;
    m_pBCI->m_slChosenFeatureSource = ChosenFeaturesOnSourceLevel;
}


//*************************************************************************************************************

void BCISetupWidget::setFilterOptions()
{
    m_pBCI->m_bUseFilter = ui.m_checkBox_UseFilter->isChecked();
    m_pBCI->m_dFilterLowerBound = ui.m_doubleSpinBox_FilterLowerBound->value();
    m_pBCI->m_dFilterUpperBound = ui.m_doubleSpinBox_FilterUpperBound->value();
    m_pBCI->m_iFilterOrder = ui.m_SpinBox_FilterOrder->value();
    m_pBCI->m_dParcksWidth = ui.m_doubleSpinBox_ParcksWidth->value();
}


//*************************************************************************************************************

void BCISetupWidget::setClassificationOptions()
{
    m_pBCI->m_iFeatureCalculationType = ui.m_comboBox_featureCalculationType->currentIndex();
}


//*************************************************************************************************************

void BCISetupWidget::showAboutDialog()
{
    BCIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}


//*************************************************************************************************************

bool BCISetupWidget::eventFilter(QObject *object, QEvent *event)
{
     if ((object == ui.m_listWidget_ChosenFeaturesOnSensorLevel ||
          object == ui.m_listWidget_ChosenFeaturesOnSourceLevel ||
          object == ui.m_listWidget_AvailableFeaturesOnSensorLevel ||
          object == ui.m_listWidget_AvailableFeaturesOnSourceLevel) && event->type() == QEvent::Leave)
         setFeatureSelection();

     return QObject::eventFilter(object, event);;
}
