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
#include <QGroupBox>


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

QuickControlWidget::QuickControlWidget(const FiffInfo::SPtr pFiffInfo,
                                       const QString& name,
                                       const QStringList& slFlags,
                                       QWidget *parent)
: DraggableFramelessWidget(parent, Qt::Window | Qt::CustomizeWindowHint)
, ui(new Ui::QuickControlWidget)
, m_pFiffInfo(pFiffInfo)
, m_slFlags(slFlags)
, m_sName(name)
{
    ui->setupUi(this);

    //Init opacity slider
    connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &QuickControlWidget::onOpacityChange);

    //Init and connect hide all group (minimize) button
    connect(ui->m_pushButton_hideAll, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::onToggleHideAll);

    connect(ui->m_pushButton_close, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::hide);

//    if(m_slFlags.contains("triggerdetection", Qt::CaseInsensitive)) {
//        createTriggerDetectionGroup();
//        m_bTriggerDetection = true;
//    } else {
//        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Trigger detection")));
//        m_bTriggerDetection = false;
//    }

//    if(m_slFlags.contains("modalities", Qt::CaseInsensitive)) {
//        createModalityGroup();
//        m_bModalitiy = true;
//    } else {
//        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Modalities")));
//        m_bModalitiy = false;
//    }

//    if(m_slFlags.contains("averages", Qt::CaseInsensitive)) {
//        createAveragesGroup();
//        m_bAverages = true;
//    } else {
//        ui->m_tabWidget_viewOptions->removeTab(ui->m_tabWidget_viewOptions->indexOf(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")));
//        m_bAverages = false;
//    }

//    if(!m_bView && !m_bTriggerDetection) {
//        ui->m_groupBox_other->hide();
//    }

    this->adjustSize();
}


//*************************************************************************************************************

QuickControlWidget::~QuickControlWidget()
{
    delete ui;
}


//*************************************************************************************************************

void QuickControlWidget::addGroupBox(QWidget* pWidget,
                                     QString sGroupBoxName)
{
    QGroupBox* pGroupBox = new QGroupBox(sGroupBoxName);
    pGroupBox->setObjectName(sGroupBoxName);

    QVBoxLayout *pVBox = new QVBoxLayout;

    pVBox->setContentsMargins(0,0,0,0);
    pVBox->addWidget(pWidget);
    pGroupBox->setLayout(pVBox);

    ui->m_gridLayout_groupBoxes->addWidget(pGroupBox,
                                 ui->m_gridLayout_groupBoxes->rowCount(),
                                 0);
}


//*************************************************************************************************************

void QuickControlWidget::addGroupBoxWithTabs(QWidget* pWidget,
                                             QString sGroupBoxName,
                                             QString sTabName)
{
    QGroupBox* pGroupBox = ui->m_widget_groupBoxes->findChild<QGroupBox *>(sGroupBoxName);

    if(!pGroupBox) {
        pGroupBox = new QGroupBox(sGroupBoxName);
        pGroupBox->setObjectName(sGroupBoxName);

        ui->m_gridLayout_groupBoxes->addWidget(pGroupBox,
                                     ui->m_gridLayout_groupBoxes->rowCount(),
                                     0);

        QVBoxLayout *pVBox = new QVBoxLayout;
        QTabWidget* pTabWidget = new QTabWidget;
        pTabWidget->setObjectName(sGroupBoxName + "TabWidget");

        pTabWidget->addTab(pWidget, sTabName);
        pVBox->setContentsMargins(4,2,4,4);
        pVBox->addWidget(pTabWidget);
        pGroupBox->setLayout(pVBox);
    } else {
        QTabWidget* pTabWidget = pGroupBox->findChild<QTabWidget *>(sGroupBoxName + "TabWidget");
        if(pTabWidget) {
            pTabWidget->addTab(pWidget, sTabName);
        }
    }
}


//*************************************************************************************************************

void QuickControlWidget::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}


//*************************************************************************************************************

void QuickControlWidget::setOpacityValue(int opactiy)
{
    //ui->m_horizontalSlider_opacity->setValue(opactiy);

    onOpacityChange(opactiy);
}


//*************************************************************************************************************

int QuickControlWidget::getOpacityValue()
{
    return ui->m_horizontalSlider_opacity->value();
}


//*************************************************************************************************************

void QuickControlWidget::setNumberDetectedTriggersAndTypes(int numberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers)
{
//    if(m_bTriggerDetection) {
//        ui->m_label_numberDetectedTriggers->setText(QString("%1").arg(numberDetections));
//    }

//    //Set trigger types
//    QMapIterator<int,QList<QPair<int,double> > > i(mapDetectedTriggers);
//    while (i.hasNext()) {
//        i.next();

//        for(int j = 0; j < i.value().size(); ++j) {
//            if(ui->m_comboBox_triggerColorType->findText(QString::number(i.value().at(j).second)) == -1) {
//                ui->m_comboBox_triggerColorType->addItem(QString::number(i.value().at(j).second));
//            }
//        }
//    }
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

void QuickControlWidget::onRealTimeTriggerActiveChanged(int state)
{
//    Q_UNUSED(state);

//    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerColorChanged(bool state)
{
//    Q_UNUSED(state);

//    QColor color = QColorDialog::getColor(m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()], this, "Set trigger color");

//    //Change color of pushbutton
//    QPalette* palette1 = new QPalette();
//    palette1->setColor(QPalette::Button,color);
//    ui->m_pushButton_triggerColor->setPalette(*palette1);
//    ui->m_pushButton_triggerColor->update();

//    m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()] = color;

//    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerThresholdChanged(double value)
{
//    Q_UNUSED(value);

//    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerColorTypeChanged(const QString &value)
{
//    //Change color of pushbutton
//    QPalette* palette1 = new QPalette();
//    palette1->setColor(QPalette::Button,m_qMapTriggerColor[value.toDouble()]);
//    ui->m_pushButton_triggerColor->setPalette(*palette1);
//    ui->m_pushButton_triggerColor->update();
}


//*************************************************************************************************************

void QuickControlWidget::onRealTimeTriggerCurrentChChanged(const QString &value)
{
//    emit triggerInfoChanged(m_qMapTriggerColor, ui->m_checkBox_activateTriggerDetection->isChecked(), ui->m_comboBox_triggerChannels->currentText(), ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));
}


//*************************************************************************************************************

void QuickControlWidget::onToggleHideAll(bool state)
{
    ui->m_widget_groupBoxes->setVisible(state);
    ui->m_widget_opacity->setVisible(state);
    this->adjustSize();
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

void QuickControlWidget::onResetTriggerNumbers()
{
//    ui->m_label_numberDetectedTriggers->setText(QString("0"));

//    emit resetTriggerCounter();

//    emit updateConnectedView();
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

void QuickControlWidget::createTriggerDetectionGroup()
{
//    //Trigger detection
//    connect(ui->m_checkBox_activateTriggerDetection, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
//            this, &QuickControlWidget::onRealTimeTriggerActiveChanged);

//    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
//        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
//            ui->m_comboBox_triggerChannels->addItem(m_pFiffInfo->chs[i].ch_name);
//        }
//    }

//    connect(ui->m_comboBox_triggerChannels, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
//            this, &QuickControlWidget::onRealTimeTriggerCurrentChChanged);

//    connect(ui->m_comboBox_triggerColorType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
//            this, &QuickControlWidget::onRealTimeTriggerColorTypeChanged);

//    connect(ui->m_pushButton_triggerColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
//            this, &QuickControlWidget::onRealTimeTriggerColorChanged);

//    ui->m_pushButton_triggerColor->setAutoFillBackground(true);
//    ui->m_pushButton_triggerColor->setFlat(true);
//    QPalette* palette1 = new QPalette();
//    palette1->setColor(QPalette::Button,QColor(177,0,0));
//    ui->m_pushButton_triggerColor->setPalette(*palette1);
//    ui->m_pushButton_triggerColor->update();

//    connect(ui->m_doubleSpinBox_detectionThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
//            this, &QuickControlWidget::onRealTimeTriggerThresholdChanged);

//    connect(ui->m_spinBox_detectionThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
//                this, &QuickControlWidget::onRealTimeTriggerThresholdChanged);

//    connect(ui->m_pushButton_resetNumberTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
//            this, &QuickControlWidget::onResetTriggerNumbers);
}


//*************************************************************************************************************

void QuickControlWidget::createModalityGroup()
{
//    m_qListModalities.clear();
//    bool hasMag = false;
//    bool hasGrad = false;
//    bool hasEEG = false;
//    bool hasEOG = false;
//    bool hasMISC = false;
//    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
//    {
//        if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
//        {
//            if(!hasMag && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T)
//                hasMag = true;
//            else if(!hasGrad &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M)
//                hasGrad = true;
//        }
//        else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH)
//            hasEEG = true;
//        else if(!hasEOG && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH)
//            hasEOG = true;
//        else if(!hasMISC && m_pFiffInfo->chs[i].kind == FIFFV_MISC_CH)
//            hasMISC = true;
//    }

//    bool sel = true;
//    float val = 1e-11f;

//    if(hasMag)
//        m_qListModalities.append(Modality("MAG",sel,val));
//    if(hasGrad)
//        m_qListModalities.append(Modality("GRAD",sel,val));
//    if(hasEEG)
//        m_qListModalities.append(Modality("EEG",false,val));
//    if(hasEOG)
//        m_qListModalities.append(Modality("EOG",false,val));
//    if(hasMISC)
//        m_qListModalities.append(Modality("MISC",false,val));

//    QGridLayout* t_pGridLayout = new QGridLayout;

//    for(qint32 i = 0; i < m_qListModalities.size(); ++i)
//    {
//        QString mod = m_qListModalities[i].m_sName;

//        QLabel* t_pLabelModality = new QLabel;
//        t_pLabelModality->setText(mod);
//        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

//        QCheckBox* t_pCheckBoxModality = new QCheckBox;
//        t_pCheckBoxModality->setChecked(m_qListModalities[i].m_bActive);
//        m_qListModalityCheckBox << t_pCheckBoxModality;
//        connect(t_pCheckBoxModality,&QCheckBox::stateChanged,
//                this,&QuickControlWidget::onUpdateModalityCheckbox);
//        t_pGridLayout->addWidget(t_pCheckBoxModality,i,1,1,1);
//    }

//    //Find Modalities tab and add current layout
//    this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Modalities")->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void QuickControlWidget::createAveragesGroup()
{
//    //Delete all widgets in the averages layout
//    QGridLayout* topLayout = static_cast<QGridLayout*>(this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")->layout());
//    if(!topLayout) {
//       topLayout = new QGridLayout();
//    }

//    QLayoutItem *child;
//    while ((child = topLayout->takeAt(0)) != 0) {
//        delete child->widget();
//        delete child;
//    }

//    //Set trigger types
//    QMapIterator<double, QPair<QColor, QPair<QString,bool> > > i(m_qMapAverageInfo);
//    int count = 0;
//    m_qMapButtonAverageType.clear();
//    m_qMapChkBoxAverageType.clear();

//    while (i.hasNext()) {
//        i.next();

//        //Create average checkbox
//        QCheckBox* pCheckBox = new QCheckBox(i.value().second.first);
//        pCheckBox->setChecked(i.value().second.second);
//        topLayout->addWidget(pCheckBox, count, 0);
//        connect(pCheckBox, &QCheckBox::clicked,
//                this, &QuickControlWidget::onAveragesChanged);
//        m_qMapChkBoxAverageType.insert(pCheckBox, i.value().second.first.toDouble());

//        //Create average color pushbutton
//        QPushButton* pButton = new QPushButton();
//        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(i.value().first.red()).arg(i.value().first.green()).arg(i.value().first.blue()));
//        topLayout->addWidget(pButton, count, 1);
//        connect(pButton, &QPushButton::clicked,
//                this, &QuickControlWidget::onAveragesChanged);
//        m_qMapButtonAverageType.insert(pButton, i.value().second.first.toDouble());

//        ++count;
//    }

//    //Find Filter tab and add current layout
//    this->findTabWidgetByText(ui->m_tabWidget_viewOptions, "Averages")->setLayout(topLayout);
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


