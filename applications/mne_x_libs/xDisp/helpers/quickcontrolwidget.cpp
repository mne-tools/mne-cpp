//=============================================================================================================
/**
* @file     projectorwidget.h
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

#include "quickcontrolwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QuickControlWidget::QuickControlWidget(QMap< qint32,float >* qMapChScaling, const FiffInfo::SPtr pFiffInfo, QWidget *parent)
: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint)
, ui(new Ui::QuickControlWidget)
, m_qMapChScaling(qMapChScaling)
, m_pFiffInfo(pFiffInfo)
{
    ui->setupUi(this);

    connect(ui->m_pushButton_hideAll, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::toggleHideAll);

    connect(ui->m_pushButton_close, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::hide);

    //Create trigger color map
    m_qMapTriggerColor.clear();

    for(int i = 0; i<pFiffInfo->chs.size(); i++) {
        if(pFiffInfo->chs[i].kind == FIFFV_STIM_CH)
            m_qMapTriggerColor.insert(pFiffInfo->chs[i].ch_name, QColor(170,0,0));
    }

    //Create different quick control groups
    createScalingGroup();

    createProjectorGroup();

    createViewGroup();

    this->adjustSize();

    this->setWindowTitle("Quick Control");

    //this->setStyleSheet("background-color: rgba(43, 125, 225, 150);");
}


//*************************************************************************************************************

QuickControlWidget::~QuickControlWidget()
{
    delete ui;
}


//*************************************************************************************************************

void QuickControlWidget::filterGroupChanged(QList<QCheckBox*> list)
{
    m_qFilterListCheckBox.clear();

    for(int u = 0; u<list.size(); u++) {
        QCheckBox* tempCheckBox = new QCheckBox(list[u]->text());
        tempCheckBox->setChecked(list[u]->isChecked());

        connect(tempCheckBox, &QCheckBox::toggled,
                list[u], &QCheckBox::setChecked);

        connect(list[u], &QCheckBox::toggled,
                tempCheckBox, &QCheckBox::setChecked);

        m_qFilterListCheckBox.append(tempCheckBox);
    }

    //Delete all widgets in the filter layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(ui->m_groupBox_filter->layout());
    if(!topLayout)
       topLayout = new QGridLayout();

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    //Add filters
    for(int u = 0; u<m_qFilterListCheckBox.size(); u++)
        topLayout->addWidget(m_qFilterListCheckBox[u], u, 0);

    ui->m_groupBox_filter->setLayout(topLayout);
}


//*************************************************************************************************************

void QuickControlWidget::createScalingGroup()
{
    QGridLayout* t_pGridLayout = new QGridLayout;

    qint32 i = 0;
    //MAG
    if(m_qMapChScaling->contains(FIFF_UNIT_T))
    {
        QLabel* t_pLabelModality = new QLabel("MAG (pT)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(500);
        t_pDoubleSpinBoxScale->setMaximumWidth(500);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFF_UNIT_T)/(1e-12));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFF_UNIT_T)/(1e-12)*10);
        m_qMapScalingSlider.insert(FIFF_UNIT_T,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //GRAD
    if(m_qMapChScaling->contains(FIFF_UNIT_T_M))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("GRAD (fT/cm)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(1);
        t_pDoubleSpinBoxScale->setMaximum(5000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFF_UNIT_T_M)/(1e-15 * 100));
        m_qMapScalingDoubleSpinBox.insert(FIFF_UNIT_T_M,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(5000);
        t_pHorizontalSlider->setSingleStep(10);
        t_pHorizontalSlider->setPageStep(10);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFF_UNIT_T_M)/(1e-15*100));
        m_qMapScalingSlider.insert(FIFF_UNIT_T_M,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EEG
    if(m_qMapChScaling->contains(FIFFV_EEG_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("EEG (uV)");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(2500);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFFV_EEG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EEG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFFV_EEG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EEG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //EOG
    if(m_qMapChScaling->contains(FIFFV_EOG_CH))
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
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFFV_EOG_CH)/(1e-06));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_EOG_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(25000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFFV_EOG_CH)/(1e-06)*10);
        m_qMapScalingSlider.insert(FIFFV_EOG_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //STIM
    if(m_qMapChScaling->contains(FIFFV_STIM_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("STIM");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(100);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFFV_STIM_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_STIM_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(1000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFFV_STIM_CH)/10);
        m_qMapScalingSlider.insert(FIFFV_STIM_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
        t_pGridLayout->addWidget(t_pHorizontalSlider,i+1,1,1,1);

        i+=2;
    }

    //MISC
    if(m_qMapChScaling->contains(FIFFV_MISC_CH))
    {
        QLabel* t_pLabelModality = new QLabel;
        t_pLabelModality->setText("MISC");
        t_pGridLayout->addWidget(t_pLabelModality,i,0,1,1);

        QDoubleSpinBox* t_pDoubleSpinBoxScale = new QDoubleSpinBox;
        t_pDoubleSpinBoxScale->setMinimum(0.1);
        t_pDoubleSpinBoxScale->setMaximum(1000);
        t_pDoubleSpinBoxScale->setMaximumWidth(100);
        t_pDoubleSpinBoxScale->setSingleStep(0.1);
        t_pDoubleSpinBoxScale->setDecimals(1);
        t_pDoubleSpinBoxScale->setPrefix("+/- ");
        t_pDoubleSpinBoxScale->setValue(m_qMapChScaling->value(FIFFV_MISC_CH));
        m_qMapScalingDoubleSpinBox.insert(FIFFV_MISC_CH,t_pDoubleSpinBoxScale);
        connect(t_pDoubleSpinBoxScale,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&QuickControlWidget::updateSpinBoxScaling);
        t_pGridLayout->addWidget(t_pDoubleSpinBoxScale,i+1,0,1,1);

        QSlider* t_pHorizontalSlider = new QSlider(Qt::Horizontal);
        t_pHorizontalSlider->setMinimum(1);
        t_pHorizontalSlider->setMaximum(10000);
        t_pHorizontalSlider->setSingleStep(1);
        t_pHorizontalSlider->setPageStep(1);
        t_pHorizontalSlider->setValue(m_qMapChScaling->value(FIFFV_MISC_CH)/10);
        m_qMapScalingSlider.insert(FIFFV_MISC_CH,t_pHorizontalSlider);
        connect(t_pHorizontalSlider,static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
                this,&QuickControlWidget::updateSliderScaling);
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
        m_qListCheckBox.clear();
        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        bool bAllActivated = true;

        int rowCount = 0;
        qint32 i=0;

        for(i; i < m_pFiffInfo->projs.size(); ++i)
        {
            QCheckBox* checkBox = new QCheckBox(m_pFiffInfo->projs[i].desc);
            checkBox->setChecked(m_pFiffInfo->projs[i].active);

            if(m_pFiffInfo->projs[i].active == false)
                bAllActivated = false;

            m_qListCheckBox.append(checkBox);

            connect(checkBox, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
                    this, &QuickControlWidget::checkStatusChanged);

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

        m_enableDisableProjectors = new QCheckBox("Enable all");
        m_enableDisableProjectors->setChecked(bAllActivated);
        topLayout->addWidget(m_enableDisableProjectors, i+2, 0);
        connect(m_enableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                this, &QuickControlWidget::enableDisableAll);

        ui->m_groupBox_projections->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void QuickControlWidget::createViewGroup()
{
    QGridLayout* t_pGridLayout = new QGridLayout;

    //Row height
    QLabel* t_pLabelModalityZoom = new QLabel("Row height:");
    t_pGridLayout->addWidget(t_pLabelModalityZoom,0,0,1,1);

    QDoubleSpinBox* t_pDoubleSpinBoxZoom = new QDoubleSpinBox;
    t_pDoubleSpinBoxZoom->setMinimum(0.3);
    t_pDoubleSpinBoxZoom->setMaximum(6.0);
    t_pDoubleSpinBoxZoom->setSingleStep(0.1);
    t_pDoubleSpinBoxZoom->setValue(1.0);
    t_pDoubleSpinBoxZoom->setSuffix(" x");
    t_pDoubleSpinBoxZoom->setToolTip(tr("Row height"));
    t_pDoubleSpinBoxZoom->setStatusTip(tr("Row height"));
    connect(t_pDoubleSpinBoxZoom, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &QuickControlWidget::zoomChanged);
    t_pGridLayout->addWidget(t_pDoubleSpinBoxZoom,0,1,1,2);

    //Window size
    QLabel* t_pLabelModality = new QLabel("Window size:");
    t_pGridLayout->addWidget(t_pLabelModality,1,0,1,1);

    QDoubleSpinBox* t_pDoubleSpinBoxWindow = new QDoubleSpinBox;
    t_pDoubleSpinBoxWindow->setMinimum(1);
    t_pDoubleSpinBoxWindow->setMaximum(10);
    t_pDoubleSpinBoxWindow->setSingleStep(1);
    t_pDoubleSpinBoxWindow->setValue(10.0);
    t_pDoubleSpinBoxWindow->setSuffix(" s");
    t_pDoubleSpinBoxWindow->setToolTip(tr("Window size"));
    t_pDoubleSpinBoxWindow->setStatusTip(tr("Window size"));
    connect(t_pDoubleSpinBoxWindow, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &QuickControlWidget::timeWindowChanged);
    t_pGridLayout->addWidget(t_pDoubleSpinBoxWindow,1,1,1,2);

    //Trigger detection
    m_pTriggerDetectionCheckBox = new QCheckBox("Trigger Detection");
    m_pTriggerDetectionCheckBox->setToolTip(tr("Real time trigger detection"));
    m_pTriggerDetectionCheckBox->setStatusTip(tr("Real time trigger detection"));
    connect(m_pTriggerDetectionCheckBox, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
            this, &QuickControlWidget::realTimeTriggerActiveChanged);
    t_pGridLayout->addWidget(m_pTriggerDetectionCheckBox,2,0,2,1);

    m_pComboBoxChannel = new QComboBox;
    QMapIterator<QString, QColor> i(m_qMapTriggerColor);
    while(i.hasNext()) {
        i.next();
        m_pComboBoxChannel->addItem(i.key());
    }
    t_pGridLayout->addWidget(m_pComboBoxChannel,2,1,1,2);
    connect(m_pComboBoxChannel, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &QuickControlWidget::realTimeTriggerCurrentChChanged);

    m_pDoubleSpinBoxThreshold = new QDoubleSpinBox;
    m_pDoubleSpinBoxThreshold->setMinimum(0.0000001);
    m_pDoubleSpinBoxThreshold->setMaximum(100000000);
    m_pDoubleSpinBoxThreshold->setSingleStep(0.01);
    m_pDoubleSpinBoxThreshold->setValue(0.01);
    m_pDoubleSpinBoxThreshold->setToolTip(tr("Trigger threshold"));
    m_pDoubleSpinBoxThreshold->setStatusTip(tr("Trigger threshold"));
    connect(m_pDoubleSpinBoxThreshold, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &QuickControlWidget::realTimeTriggerThresholdChanged);
    t_pGridLayout->addWidget(m_pDoubleSpinBoxThreshold,3,1,1,1);

    m_pTriggerColorButton = new QPushButton;
    m_pTriggerColorButton->setText("Change");
    m_pTriggerColorButton->setToolTip(tr("Trigger color"));
    m_pTriggerColorButton->setStatusTip(tr("Toggle trigger color"));
    connect(m_pTriggerColorButton, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlWidget::realTimeTriggerColorChanged);
    m_pTriggerColorButton->setAutoFillBackground(true);
    m_pTriggerColorButton->setFlat(true);
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,QColor(255,20,20));
    m_pTriggerColorButton->setPalette(*palette1);
    m_pTriggerColorButton->update();
    t_pGridLayout->addWidget(m_pTriggerColorButton,3,2,1,1);

    ui->m_groupBox_view->setLayout(t_pGridLayout);
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

void QuickControlWidget::checkStatusChanged(int status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListCheckBox.size(); ++i) {
        if(m_qListCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListCheckBox[i]->isChecked();
    }

    m_enableDisableProjectors->setChecked(bAllActivated);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void QuickControlWidget::enableDisableAll(bool status)
{
    for(int i=0; i<m_qListCheckBox.size(); i++)
        m_qListCheckBox.at(i)->setChecked(status);
}


//*************************************************************************************************************

void QuickControlWidget::updateSpinBoxScaling(double value)
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

        m_qMapChScaling->insert(it.key(), it.value()->value() * scaleValue);
//        qDebug()<<"m_pRTMSAW->m_qMapChScaling[it.key()]" << m_pRTMSAW->m_qMapChScaling[it.key()];
    }

    emit scalingChanged();
}


//*************************************************************************************************************

void QuickControlWidget::updateSliderScaling(int value)
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
}


//*************************************************************************************************************

void QuickControlWidget::realTimeTriggerActiveChanged(int state)
{
    Q_UNUSED(state);

    emit triggerInfoChanged(m_qMapTriggerColor, m_pTriggerDetectionCheckBox->isChecked(), m_pComboBoxChannel->currentText(), m_pDoubleSpinBoxThreshold->value());
}


//*************************************************************************************************************

void QuickControlWidget::realTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    QColor color = QColorDialog::getColor(m_qMapTriggerColor[m_pComboBoxChannel->currentText()], this, "Set trigger color");

    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,color);
    m_pTriggerColorButton->setPalette(*palette1);
    m_pTriggerColorButton->update();

    m_qMapTriggerColor[m_pComboBoxChannel->currentText()] = color;

    emit triggerInfoChanged(m_qMapTriggerColor, m_pTriggerDetectionCheckBox->isChecked(), m_pComboBoxChannel->currentText(), m_pDoubleSpinBoxThreshold->value());
}


//*************************************************************************************************************

void QuickControlWidget::realTimeTriggerThresholdChanged(double value)
{
    Q_UNUSED(value);

    emit triggerInfoChanged(m_qMapTriggerColor, m_pTriggerDetectionCheckBox->isChecked(), m_pComboBoxChannel->currentText(), m_pDoubleSpinBoxThreshold->value());
}


//*************************************************************************************************************

void QuickControlWidget::realTimeTriggerCurrentChChanged(const QString &value)
{
    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,m_qMapTriggerColor[value]);
    m_pTriggerColorButton->setPalette(*palette1);
    m_pTriggerColorButton->update();

    emit triggerInfoChanged(m_qMapTriggerColor, m_pTriggerDetectionCheckBox->isChecked(), m_pComboBoxChannel->currentText(), m_pDoubleSpinBoxThreshold->value());
}


//*************************************************************************************************************

void QuickControlWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}


//*************************************************************************************************************

void QuickControlWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}


//*************************************************************************************************************

void QuickControlWidget::resizeEvent(QResizeEvent * /* event */)
{
    setMask(roundedRect(QRect(0,0,width(),height()),10));
}


//*************************************************************************************************************

QRegion QuickControlWidget::roundedRect(const QRect& rect, int r)
{
    QRegion region;
    // middle and borders
    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);
    // top left
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);
    // top right
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom left
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom right
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Ellipse);
    return region;
}


//*************************************************************************************************************

void QuickControlWidget::toggleHideAll(bool state)
{
    if(!state) {
        ui->m_groupBox_projections->hide();
        ui->m_groupBox_filter->hide();
        ui->m_groupBox_scaling->hide();
        ui->m_groupBox_view->hide();
        ui->m_pushButton_hideAll->setText("Maximize - Quick Control");
    }
    else {
        ui->m_groupBox_projections->show();
        ui->m_groupBox_filter->show();
        ui->m_groupBox_scaling->show();
        ui->m_groupBox_view->show();
        ui->m_pushButton_hideAll->setText("Minimize - Quick Control");
    }

    this->adjustSize();
    this->resize(width(), ui->m_pushButton_hideAll->height()-50);
}


