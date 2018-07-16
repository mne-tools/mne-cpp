//=============================================================================================================
/**
* @file     quickcontrolwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the QuickControlWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_quickcontrolwidget.h"
#include "quickcontrolwidget.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QSignalMapper>
#include <QColorDialog>
#include <QTabWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace FIFFLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QuickControlWidget::QuickControlWidget(const QMap<qint32,
                                       float>& qMapChScaling,
                                       const FiffInfo::SPtr pFiffInfo,
                                       const QString& name,
                                       const QStringList& slFlags,
                                       QWidget *parent)
: DraggableFramelessWidget(parent, Qt::Window | Qt::CustomizeWindowHint)
, ui(new Ui::QuickControlWidget)
, m_qMapChScaling(qMapChScaling)
, m_pFiffInfo(pFiffInfo)
, m_slFlags(slFlags)
, m_sName(name)
, m_pEnableDisableProjectors(Q_NULLPTR)
, m_pShowFilterOptions(Q_NULLPTR)
, m_pCompSignalMapper(Q_NULLPTR)
{
    ui->setupUi(this);

    //Init and connect hide all group (minimize) button
    ui->m_pushButton_hideAll->setText(ui->m_pushButton_hideAll->text().append(QString(" - %1").arg(m_sName)));
    connect(ui->m_pushButton_hideAll, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onToggleHideAll);

    connect(ui->m_pushButton_close, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::hide);

    //Connect screenshot button
    connect(ui->m_pushButton_makeScreenshot, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onMakeScreenshot);

    //Init bool flags from string list and create groups respectivley
    if(m_slFlags.contains("scaling", Qt::CaseInsensitive)) {
        createScalingGroup();
        m_bScaling = true;
    } else {
        ui->m_groupBox_scaling->hide();
        m_bScaling = false;
    }

    if(m_slFlags.contains("projections", Qt::CaseInsensitive)) {
        createProjectorGroup();
        m_bProjections = true;
    } else {        
        ui->m_tabWidget_noiseReduction->removeTab(ui->m_tabWidget_noiseReduction->indexOf(this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "SSP")));
        m_bProjections = false;
    }

    if(m_slFlags.contains("compensators", Qt::CaseInsensitive)) {
        createCompensatorGroup();
        m_bCompensator = true;
    } else {
        ui->m_tabWidget_noiseReduction->removeTab(ui->m_tabWidget_noiseReduction->indexOf(this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "Comp")));
        m_bCompensator = false;
    }

    if(m_slFlags.contains("sphara", Qt::CaseInsensitive)) {
        m_bSphara = true;
        createSpharaGroup();
    } else {
        ui->m_tabWidget_noiseReduction->removeTab(ui->m_tabWidget_noiseReduction->indexOf(this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "SPHARA")));
        m_bSphara = false;
    }

    if(m_slFlags.contains("filter", Qt::CaseInsensitive)) {
        m_bFilter = true;
    } else {
        ui->m_tabWidget_noiseReduction->removeTab(ui->m_tabWidget_noiseReduction->indexOf(this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "Filter")));
        m_bFilter = false;
    }

    if(m_slFlags.contains("view", Qt::CaseInsensitive)) {
        createViewGroup();
        m_bView = true;
    } else {
        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "View")));
        m_bView = false;
    }

    if(m_slFlags.contains("colors", Qt::CaseInsensitive)) {
        createColorsGroup();
        m_bView = true;
    } else {
        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Colors")));
        m_bView = false;
    }

    if(m_slFlags.contains("triggerdetection", Qt::CaseInsensitive)) {
        createTriggerDetectionGroup();
        m_bTriggerDetection = true;
    } else {
        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Trigger detection")));
        m_bTriggerDetection = false;
    }

    if(m_slFlags.contains("modalities", Qt::CaseInsensitive)) {
        createModalityGroup();
        m_bModalitiy = true;
    } else {
        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Modalities")));
        m_bModalitiy = false;
    }

    if(m_slFlags.contains("averages", Qt::CaseInsensitive)) {
        createAveragesGroup();
        m_bAverages = true;
    } else {
        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")));
        m_bAverages = false;
    }

    //Decide whether to complete hide some groups
    if(!m_bFilter && !m_bProjections && !m_bCompensator && !m_bSphara) {
        ui->m_groupBox_noise->hide();
    }

    if(!m_bView && !m_bTriggerDetection) {
        ui->m_groupBox_other->hide();
    }

    this->adjustSize();
}


//*************************************************************************************************************

QuickControlWidget::~QuickControlWidget()
{
    delete ui;
}


//*************************************************************************************************************

void QuickControlWidget::filterGroupChanged(QList<QCheckBox*> list)
{
    if(m_bFilter) {
        m_qFilterListCheckBox.clear();

        for(int u = 0; u < list.size(); ++u) {
            QCheckBox* tempCheckBox = new QCheckBox(list[u]->text());
            tempCheckBox->setChecked(list[u]->isChecked());

            connect(tempCheckBox, &QCheckBox::toggled,
                    list[u], &QCheckBox::setChecked);

            if(tempCheckBox->text() == "Activate user designed filter")
                connect(tempCheckBox, &QCheckBox::toggled,
                        this, &QuickControlWidget::onUserFilterToggled);

            connect(list[u], &QCheckBox::toggled,
                    tempCheckBox, &QCheckBox::setChecked);

            m_qFilterListCheckBox.append(tempCheckBox);
        }

        //Delete all widgets in the filter layout
        QGridLayout* topLayout = static_cast<QGridLayout*>(this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "Filter")->layout());
        if(!topLayout) {
           topLayout = new QGridLayout();
        }

        QLayoutItem *child;
        while ((child = topLayout->takeAt(0)) != 0) {
            delete child->widget();
            delete child;
        }

        //Add filters
        int u = 0;

        for(u; u < m_qFilterListCheckBox.size(); ++u) {
            topLayout->addWidget(m_qFilterListCheckBox[u], u, 0);
        }

        //Add push button for filter options
        m_pShowFilterOptions = new QPushButton();
//        m_pShowFilterOptions->setText("Open Filter options");
        m_pShowFilterOptions->setText("Filter options");
        m_pShowFilterOptions->setCheckable(false);
        connect(m_pShowFilterOptions, &QPushButton::clicked,
                this, &QuickControlWidget::onShowFilterOptions);

        topLayout->addWidget(m_pShowFilterOptions, u+1, 0);

        //Find Filter tab and add current layout
        this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "Filter")->setLayout(topLayout);

        //createViewGroup();
    }
}


//*************************************************************************************************************

void QuickControlWidget::setViewParameters(double zoomFactor, int windowSize, int opactiy)
{
    ui->m_doubleSpinBox_numberVisibleChannels->setValue(zoomFactor);
    ui->m_spinBox_windowSize->setValue(windowSize);
    ui->m_horizontalSlider_opacity->setValue(opactiy);

    zoomChanged(zoomFactor);
    timeWindowChanged(windowSize);
    onOpacityChange(opactiy);
}


//*************************************************************************************************************

int QuickControlWidget::getOpacityValue()
{
    return ui->m_horizontalSlider_opacity->value();
}


//*************************************************************************************************************

int QuickControlWidget::getDistanceTimeSpacerIndex()
{
    return ui->m_comboBox_distaceTimeSpacer->currentIndex();
}


//*************************************************************************************************************

void QuickControlWidget::setDistanceTimeSpacerIndex(int index)
{
    ui->m_comboBox_distaceTimeSpacer->setCurrentIndex(index);
}


//*************************************************************************************************************

void QuickControlWidget::setSignalBackgroundColors(const QColor& signalColor, const QColor& backgroundColor)
{
    ui->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(backgroundColor.red()).arg(backgroundColor.green()).arg(backgroundColor.blue()));
    ui->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(signalColor.red()).arg(signalColor.green()).arg(signalColor.blue()));

    m_colCurrentBackgroundColor = backgroundColor;
    m_colCurrentSignalColor = signalColor;
}


//*************************************************************************************************************

void QuickControlWidget::setNumberDetectedTriggersAndTypes(int numberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers)
{
    if(m_bTriggerDetection) {
        ui->m_label_numberDetectedTriggers->setText(QString("%1").arg(numberDetections));
    }

    //Set trigger types
    QMapIterator<int,QList<QPair<int,double> > > i(mapDetectedTriggers);
    while (i.hasNext()) {
        i.next();

        for(int j = 0; j < i.value().size(); ++j) {
            if(ui->m_comboBox_triggerColorType->findText(QString::number(i.value().at(j).second)) == -1) {
                ui->m_comboBox_triggerColorType->addItem(QString::number(i.value().at(j).second));
            }
        }
    }
}


//*************************************************************************************************************

const QColor& QuickControlWidget::getSignalColor()
{
    return m_colCurrentSignalColor;
}


//*************************************************************************************************************

const QColor& QuickControlWidget::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}


//*************************************************************************************************************

void QuickControlWidget::setAverageInformationMapOld(const QMap<double, QPair<QColor, QPair<QString,bool> > >& qMapAverageInfoOld)
{
    m_qMapAverageInfoOld = qMapAverageInfoOld;
}


//*************************************************************************************************************

void QuickControlWidget::setAverageInformationMap(const QMap<double, QPair<QColor, QPair<QString,bool> > >& qMapAverageColor)
{
    //Check if average type already exists in the map
    QMapIterator<double, QPair<QColor, QPair<QString,bool> > > i(qMapAverageColor);

    while (i.hasNext()) {
        i.next();

        if(!m_qMapAverageInfo.contains(i.key())) {
            if(m_qMapAverageInfoOld.contains(i.key())) {
                //Use old color
                QPair<QColor, QPair<QString,bool> > tempPair = i.value();
                tempPair.first = m_qMapAverageInfoOld[i.key()].first;

                m_qMapAverageInfo.insert(i.key(), tempPair);
            } else {
                //Use default color
                m_qMapAverageInfo.insert(i.key(), i.value());
            }
        }
    }

    //Recreate average group
    if(m_bAverages) {
        createAveragesGroup();

        emit averageInformationChanged(m_qMapAverageInfo);
    }
}


//*************************************************************************************************************

QMap<double, QPair<QColor, QPair<QString,bool> > > QuickControlWidget::getAverageInformationMap()
{
    return m_qMapAverageInfo;
}


//*************************************************************************************************************

void QuickControlWidget::onTimeWindowChanged(int value)
{
    emit timeWindowChanged(value);
}


//*************************************************************************************************************

void QuickControlWidget::onZoomChanged(double value)
{
    emit zoomChanged(value);
}


//*************************************************************************************************************

void QuickControlWidget::onCheckProjStatusChanged(bool status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if(m_qListProjCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListProjCheckBox[i]->isChecked();
    }

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(bAllActivated);
    }

    emit projSelectionChanged();

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onEnableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for(int i=0; i<m_qListProjCheckBox.size(); i++)
        m_qListProjCheckBox.at(i)->setChecked(status);

    //Set all projection activation states to status
    for(int i=0; i < m_pFiffInfo->projs.size(); ++i)
        m_pFiffInfo->projs[i].active = status;

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(status);
    }

    emit projSelectionChanged();

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onCheckCompStatusChanged(const QString & compName)
{
    //qDebug()<<compName;

    bool currentState = false;

    for(int i = 0; i < m_qListCompCheckBox.size(); ++i) {
        if(m_qListCompCheckBox[i]->text() != compName) {
            m_qListCompCheckBox[i]->setChecked(false);
        } else {
            currentState = m_qListCompCheckBox[i]->isChecked();
        }
    }

    if(currentState) {
        emit compSelectionChanged(compName.toInt());
    } else { //If none selected
        emit compSelectionChanged(0);
    }

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onUpdateSpinBoxScaling(double value)
{
    Q_UNUSED(value)

    QMap<qint32, QDoubleSpinBox*>::iterator it;
    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
    {
        double scaleValue = 0;

        switch(it.key())
        {
            case FIFF_UNIT_T:
                //MAG
                scaleValue = 1e-12;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFF_UNIT_T_M:
                //GRAD
                scaleValue = 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*1);
                break;
            case FIFFV_EEG_CH:
                //EEG
                scaleValue = 1e-06;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_EOG_CH:
                //EOG
                scaleValue = 1e-06;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_EMG_CH:
                //EMG
                scaleValue = 1e-03;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_ECG_CH:
                //ECG
                scaleValue = 1e-03;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_MISC_CH:
                //MISC
                scaleValue = 1;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            case FIFFV_STIM_CH:
                //STIM
                scaleValue = 1;
                m_qMapScalingSlider[it.key()]->setValue(it.value()->value()*10);
                break;
            default:
                scaleValue = 1.0;
        }

        //if(m_qMapScalingSlider[it.key()]->maximum()<it.value()->value()*10)
            m_qMapChScaling.insert(it.key(), it.value()->value() * scaleValue);
//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }

    emit scalingChanged(m_qMapChScaling);

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onUpdateSliderScaling(int value)
{
    Q_UNUSED(value)

    QMap<qint32, QDoubleSpinBox*>::iterator it;
    for (it = m_qMapScalingDoubleSpinBox.begin(); it != m_qMapScalingDoubleSpinBox.end(); ++it)
    {
        double scaleValue = 0;

        switch(it.key())
        {
            case FIFF_UNIT_T:
                //MAG
                scaleValue = 1e-12;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFF_UNIT_T_M:
                //GRAD
                scaleValue = 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/1);
                break;
            case FIFFV_EEG_CH:
                //EEG
                scaleValue = 1e-06;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_EOG_CH:
                //EOG
                scaleValue = 1e-06;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_EMG_CH:
                //EMG
                scaleValue = 1e-03;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_ECG_CH:
                //ECG
                scaleValue = 1e-03;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_MISC_CH:
                //MISC
                scaleValue = 1;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            case FIFFV_STIM_CH:
                //STIM
                scaleValue = 1;
                it.value()->setValue((double)m_qMapScalingSlider[it.key()]->value()/10);
                break;
            default:
                scaleValue = 1.0;
        }

//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }

//    emit scalingChanged();

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerActiveChanged(int state)
{
    Q_UNUSED(state);

    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    QColor color = QColorDialog::getColor(m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()], this, "Set trigger color");

    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,color);
    ui->m_pushButton_triggerColor->setPalette(*palette1);
    ui->m_pushButton_triggerColor->update();

    m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()] = color;

    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerThresholdChanged(double value)
{
    Q_UNUSED(value);

    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerColorTypeChanged(const QString &value)
{
    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,m_qMapTriggerColor[value.toDouble()]);
    ui->m_pushButton_triggerColor->setPalette(*palette1);
    ui->m_pushButton_triggerColor->update();
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerCurrentChChanged(const QString &value)
{
    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onToggleHideAll(bool state)
{
    if(!state) {
        //ui->m_widget_master->hide();
        ui->m_groupBox_noise->hide();
        ui->m_groupBox_other->hide();
        ui->m_groupBox_scaling->hide();

        ui->m_pushButton_hideAll->setText(QString("Maximize - Quick Control - %1").arg(m_sName));
    }
    else {
        ui->m_groupBox_scaling->show();

        if(m_bFilter || m_bProjections || m_bCompensator || m_bSphara) {
            ui->m_groupBox_noise->show();
        }

        if(m_bView || m_bTriggerDetection) {
            ui->m_groupBox_other->show();
        }

        ui->m_pushButton_hideAll->setText(QString("Minimize - Quick Control - %1").arg(m_sName));
    }

    this->adjustSize();
    this->resize(width(), ui->m_pushButton_hideAll->height()-150);
}


//*************************************************************************************************************

void QuickControlWidget::onShowFilterOptions(bool state)
{
//    if(state)
//        m_pShowFilterOptions->setText("Close filter options");
//    else
//        m_pShowFilterOptions->setText("Open filter options");

//    m_pShowFilterOptions->setChecked(state);

//    emit showFilterOptions(state);

    Q_UNUSED(state);
    emit showFilterOptions(true);
}


//*************************************************************************************************************

void QuickControlWidget::onUpdateModalityCheckbox(qint32 state)
{
    Q_UNUSED(state)

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i)
    {
        if(m_qListModalityCheckBox[i]->isChecked())
            m_qListModalities[i].m_bActive = true;
        else
            m_qListModalities[i].m_bActive = false;
    }

    emit modalitiesChanged(m_qListModalities);

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}


//*************************************************************************************************************

void QuickControlWidget::onDistanceTimeSpacerChanged(qint32 value)
{
    switch(value) {
        case 0:
            emit distanceTimeSpacerChanged(100);
        break;

        case 1:
            emit distanceTimeSpacerChanged(200);
        break;

        case 2:
            emit distanceTimeSpacerChanged(500);
        break;

        case 3:
            emit distanceTimeSpacerChanged(1000);
        break;
    }

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onResetTriggerNumbers()
{
    ui->m_label_numberDetectedTriggers->setText(QString("0"));

    emit resetTriggerCounter();

    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onUserFilterToggled(bool state)
{
    Q_UNUSED(state);
    //qDebug()<<"onUserFilterToggled";
    emit updateConnectedView();
}


//*************************************************************************************************************

void QuickControlWidget::onSpharaButtonClicked(bool state)
{
    emit spharaActivationChanged(state);
}


//*************************************************************************************************************

void QuickControlWidget::onSpharaOptionsChanged()
{
    ui->m_label_spharaFirst->show();
    ui->m_spinBox_spharaFirst->show();

    ui->m_label_spharaSecond->show();
    ui->m_spinBox_spharaSecond->show();

    if(ui->m_comboBox_spharaSystem->currentText() == "VectorView") {
        ui->m_label_spharaFirst->setText("Mag");
        ui->m_spinBox_spharaFirst->setMaximum(102);

        ui->m_label_spharaSecond->setText("Grad");
        ui->m_spinBox_spharaSecond->setMaximum(102);
    }

    if(ui->m_comboBox_spharaSystem->currentText() == "BabyMEG") {
        ui->m_label_spharaFirst->setText("Inner layer");
        ui->m_spinBox_spharaFirst->setMaximum(270);

        ui->m_label_spharaSecond->setText("Outer layer");
        ui->m_spinBox_spharaSecond->setMaximum(105);
    }

    if(ui->m_comboBox_spharaSystem->currentText() == "EEG") {
        ui->m_label_spharaFirst->setText("EEG");
        ui->m_spinBox_spharaFirst->setMaximum(256);

        ui->m_label_spharaSecond->hide();
        ui->m_spinBox_spharaSecond->hide();
    }

    emit spharaOptionsChanged(ui->m_comboBox_spharaSystem->currentText(), ui->m_spinBox_spharaFirst->value(), ui->m_spinBox_spharaSecond->value());
}


//*************************************************************************************************************

void QuickControlWidget::onViewColorButtonClicked()
{
    QColorDialog* pDialog = new QColorDialog(this);

    QObject* obj = sender();
    if(obj == ui->m_pushButton_signalColor) {
        pDialog->setCurrentColor(m_colCurrentSignalColor);
        pDialog->setWindowTitle("Select Signal Color");

        pDialog->exec();
        m_colCurrentSignalColor = pDialog->currentColor();

        //Set color of button new new scene color
        ui->m_pushButton_signalColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentSignalColor.red()).arg(m_colCurrentSignalColor.green()).arg(m_colCurrentSignalColor.blue()));

        emit signalColorChanged(m_colCurrentSignalColor);
    }

    if( obj == ui->m_pushButton_backgroundColor ) {
        pDialog->setCurrentColor(m_colCurrentBackgroundColor);
        pDialog->setWindowTitle("Select Background Color");

        pDialog->exec();
        m_colCurrentBackgroundColor = pDialog->currentColor();

        //Set color of button new new scene color
        ui->m_pushButton_backgroundColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentBackgroundColor.red()).arg(m_colCurrentBackgroundColor.green()).arg(m_colCurrentBackgroundColor.blue()));

        emit backgroundColorChanged(m_colCurrentBackgroundColor);
    }
}


//*************************************************************************************************************

void QuickControlWidget::onMakeScreenshot()
{
    qDebug()<<ui->m_comboBox_imageType->currentText();
    emit makeScreenshot(ui->m_comboBox_imageType->currentText());
}


//*************************************************************************************************************

void QuickControlWidget::onAveragesChanged()
{
    //Change color for average
    if(QPushButton* button = qobject_cast<QPushButton*>(sender()))
    {
        QColor color = QColorDialog::getColor(m_qMapAverageInfo[m_qMapButtonAverageType[button]].first, this, "Set average color");

        //Change color of pushbutton
        QPalette* palette1 = new QPalette();
        palette1->setColor(QPalette::Button,color);
        button->setPalette(*palette1);
        button->update();

        //Set color of button new new scene color
        button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));

        m_qMapAverageInfo[m_qMapButtonAverageType[button]].first = color;

        emit averageInformationChanged(m_qMapAverageInfo);
    }

    //Change color for average
    if(QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender()))
    {
        m_qMapAverageInfo[m_qMapChkBoxAverageType[checkBox]].second.second = checkBox->isChecked();

        emit averageInformationChanged(m_qMapAverageInfo);
    }
}


//*************************************************************************************************************

void QuickControlWidget::createScalingGroup()
{
    QGridLayout* t_pGridLayout = new QGridLayout;

    qint32 i = 0;
    //MAG
    if(m_qMapChScaling.contains(FIFF_UNIT_T))
    {
        QLabel* t_pLabelModality = new QLabel("MAG (pT)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(50000);
        t_pDoubleSpinBoxScale->setMaximumWidth(500);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(1e-12));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T)/(1e-12)*10);
        m_qMapScalingSlider.insert(FIFF_UNIT_T,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //GRAD
    if(m_qMapChScaling.contains(FIFF_UNIT_T_M))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD (fT/cm)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(1);
        t_pDoubleSpinBoxScale->setMaximum(500000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(1e-15 * 100));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(10);
        t_pHorizontalSlider->setPageStep(10);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFF_UNIT_T_M)/(1e-15*100));
        m_qMapScalingSlider.insert(FIFF_UNIT_T_M,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EEG
    if(m_qMapChScaling.contains(FIFFV_EEG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(25000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EEG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EEG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EOG
    if(m_qMapChScaling.contains(FIFFV_EOG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EOG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(102500e14);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_EOG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EOG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //STIM
    if(m_qMapChScaling.contains(FIFFV_STIM_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.001);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.001);
        t_pDoubleSpinBoxScale->setDecimals(3);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_STIM_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_STIM_CH)*10);
        m_qMapScalingSlider.insert(FIFFV_STIM_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);


        i+=2;
    }

    //MISC
    if(m_qMapChScaling.contains(FIFFV_MISC_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(10000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling.value(FIFFV_MISC_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::onUpdateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(10000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling.value(FIFFV_MISC_CH)/10);
        m_qMapScalingSlider.insert(FIFFV_MISC_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::onUpdateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    ui->m_groupBox_scaling->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void QuickControlWidget::createProjectorGroup()
{
    if(m_pFiffInfo)
    {
        //If no projectors are defined return here
        if(m_pFiffInfo->projs.empty()) {
            return;
        }

        m_qListProjCheckBox.clear();
        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        bool bAllActivated = true;

        qint32 i=0;

        for(i; i < m_pFiffInfo->projs.size(); ++i)
        {
            QCheckBox* checkBox = new QCheckBox(m_pFiffInfo->projs[i].desc);
            checkBox->setChecked(m_pFiffInfo->projs[i].active);

            if(m_pFiffInfo->projs[i].active == false)
                bAllActivated = false;

            m_qListProjCheckBox.append(checkBox);

            connect(checkBox, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                    this, &QuickControlWidget::onCheckProjStatusChanged);

            topLayout->addWidget(checkBox, i, 0); //+2 because we already added two widgets before the first projector check box

//            if(i>m_pFiffInfo->projs.size()/2)
//                topLayout->addWidget(checkBox, i-rowCount, 1); //+2 because we already added two widgets before the first projector check box
//            else {
//                topLayout->addWidget(checkBox, i, 0); //+2 because we already added two widgets before the first projector check box
//                rowCount++;
//            }
        }

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        topLayout->addWidget(line, i+1, 0);

        m_pEnableDisableProjectors = new QCheckBox("Enable all");
        m_pEnableDisableProjectors->setChecked(bAllActivated);
        topLayout->addWidget(m_pEnableDisableProjectors, i+2, 0);
        connect(m_pEnableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &QuickControlWidget::onEnableDisableAllProj);

        //Find SSP tab and add current layout
        this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "SSP")->setLayout(topLayout);

        //Set default activation to true
        onEnableDisableAllProj(true);
    }
}


//*************************************************************************************************************

void QuickControlWidget::createSpharaGroup()
{
    //Sphara activation changed
    connect(ui->m_checkBox_activateSphara, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &QuickControlWidget::onSpharaButtonClicked);

    //Sphara options changed
    connect(ui->m_comboBox_spharaSystem, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &QuickControlWidget::onSpharaOptionsChanged);

    connect(ui->m_spinBox_spharaFirst, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &QuickControlWidget::onSpharaOptionsChanged);

    connect(ui->m_spinBox_spharaSecond, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &QuickControlWidget::onSpharaOptionsChanged);
}


//*************************************************************************************************************

void QuickControlWidget::createViewGroup()
{
    //Number of visible channels
    connect(ui->m_doubleSpinBox_numberVisibleChannels, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &QuickControlWidget::zoomChanged);

    //Window size
    connect(ui->m_spinBox_windowSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &QuickControlWidget::timeWindowChanged);

    //opacity
    connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &QuickControlWidget::onOpacityChange);

    //Distance for timer spacer
    connect(ui->m_comboBox_distaceTimeSpacer, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &QuickControlWidget::onDistanceTimeSpacerChanged);
}


//*************************************************************************************************************

void QuickControlWidget::createColorsGroup()
{
    //Colors
    connect(ui->m_pushButton_backgroundColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onViewColorButtonClicked);

    connect(ui->m_pushButton_signalColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onViewColorButtonClicked);
}


//*************************************************************************************************************

void QuickControlWidget::createTriggerDetectionGroup()
{
    //Trigger detection
    connect(ui->m_checkBox_activateTriggerDetection, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
            this, &QuickControlWidget::onRealTimeTriggerActiveChanged);

    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
            ui->m_comboBox_triggerChannels->addItem(m_pFiffInfo->chs[i].ch_name);
        }
    }

    connect(ui->m_comboBox_triggerChannels, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &QuickControlWidget::onRealTimeTriggerCurrentChChanged);

    connect(ui->m_comboBox_triggerColorType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &QuickControlWidget::onRealTimeTriggerColorTypeChanged);

    connect(ui->m_pushButton_triggerColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onRealTimeTriggerColorChanged);

    ui->m_pushButton_triggerColor->setAutoFillBackground(true);
    ui->m_pushButton_triggerColor->setFlat(true);
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,QColor(177,0,0));
    ui->m_pushButton_triggerColor->setPalette(*palette1);
    ui->m_pushButton_triggerColor->update();

    connect(ui->m_doubleSpinBox_detectionThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &QuickControlWidget::onRealTimeTriggerThresholdChanged);

    connect(ui->m_spinBox_detectionThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &QuickControlWidget::onRealTimeTriggerThresholdChanged);

    connect(ui->m_pushButton_resetNumberTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onResetTriggerNumbers);
}


//*************************************************************************************************************

void QuickControlWidget::createModalityGroup()
{
    m_qListModalities.clear();
    bool hasMag = false;
    bool hasGrad = false;
    bool hasEEG = false;
    bool hasEOG = false;
    bool hasMISC = false;
    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
    {
        if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
        {
            if(!hasMag && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T)
                hasMag = true;
            else if(!hasGrad &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M)
                hasGrad = true;
        }
        else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH)
            hasEEG = true;
        else if(!hasEOG && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH)
            hasEOG = true;
        else if(!hasMISC && m_pFiffInfo->chs[i].kind == FIFFV_MISC_CH)
            hasMISC = true;
    }

    bool sel = true;
    float val = 1e-11f;

    if(hasMag)
        m_qListModalities.append(Modality("MAG",sel,val));
    if(hasGrad)
        m_qListModalities.append(Modality("GRAD",sel,val));
    if(hasEEG)
        m_qListModalities.append(Modality("EEG",false,val));
    if(hasEOG)
        m_qListModalities.append(Modality("EOG",false,val));
    if(hasMISC)
        m_qListModalities.append(Modality("MISC",false,val));

    QGridLayout* t_pGridLayout = new QGridLayout;

    for(qint32 i = 0; i < m_qListModalities.size(); ++i)
    {
        QString mod = m_qListModalities[i].m_sName;

        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText(mod);
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QCheckBox* t_pCheckBoxModality = new QCheckBox;
        t_pCheckBoxModality->setChecked(m_qListModalities[i].m_bActive);
        m_qListModalityCheckBox << t_pCheckBoxModality;
        connect(t_pCheckBoxModality,&QCheckBox::stateChanged,
                this,&QuickControlWidget::onUpdateModalityCheckbox);
        t_pGridLayout->addWidget(t_pCheckBoxModality,i,1,1,1);
    }

    //Find Modalities tab and add current layout
    this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Modalities")->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void QuickControlWidget::createCompensatorGroup()
{
    if(m_pFiffInfo)
    {
        m_pCompSignalMapper = new QSignalMapper(this);

        m_qListCompCheckBox.clear();

        // Compensation Selection
        QGridLayout *topLayout = new QGridLayout;

        qint32 i=0;

        for(i; i < m_pFiffInfo->comps.size(); ++i)
        {
            QString numStr;
            QCheckBox* checkBox = new QCheckBox(numStr.setNum(m_pFiffInfo->comps[i].kind));

            m_qListCompCheckBox.append(checkBox);

            connect(checkBox, SIGNAL(clicked()),
                        m_pCompSignalMapper, SLOT(map()));

            m_pCompSignalMapper->setMapping(checkBox, numStr);

            topLayout->addWidget(checkBox, i, 0);

        }

        connect(m_pCompSignalMapper, SIGNAL(mapped(const QString &)),
                    this, SIGNAL(compClicked(const QString &)));

        connect(this, &QuickControlWidget::compClicked,
                this, &QuickControlWidget::onCheckCompStatusChanged);

        //Find Comp tab and add current layout
        this->findTabWidgetByText(ui->m_tabWidget_noiseReduction, "Comp")->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void QuickControlWidget::createAveragesGroup()
{
    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    //Set trigger types
    QMapIterator<double, QPair<QColor, QPair<QString,bool> > > i(m_qMapAverageInfo);
    int count = 0;
    m_qMapButtonAverageType.clear();
    m_qMapChkBoxAverageType.clear();

    while (i.hasNext()) {
        i.next();

        //Create average checkbox
        QCheckBox* pCheckBox = new QCheckBox(i.value().second.first);
        pCheckBox->setChecked(i.value().second.second);
        topLayout->addWidget(pCheckBox, count, 0);
        connect(pCheckBox, &QCheckBox::clicked,
                this, &QuickControlWidget::onAveragesChanged);
        m_qMapChkBoxAverageType.insert(pCheckBox, i.value().second.first.toDouble());

        //Create average color pushbutton
        QPushButton* pButton = new QPushButton();
        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(i.value().first.red()).arg(i.value().first.green()).arg(i.value().first.blue()));
        topLayout->addWidget(pButton, count, 1);
        connect(pButton, &QPushButton::clicked,
                this, &QuickControlWidget::onAveragesChanged);
        m_qMapButtonAverageType.insert(pButton, i.value().second.first.toDouble());

        ++count;
    }

    //Find Filter tab and add current layout
    this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")->setLayout(topLayout);
}


//*************************************************************************************************************

QWidget* QuickControlWidget::findTabWidgetByText(const QTabWidget* pTabWidget, const QString& sTabText)
{
    for(int i = 0; i<pTabWidget->count(); i++) {
        if(pTabWidget->tabText(i) == sTabText) {
            return pTabWidget->widget(i);
        }
    }

    return new QWidget();
}


