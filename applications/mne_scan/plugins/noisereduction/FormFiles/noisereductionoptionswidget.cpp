//=============================================================================================================
/**
* @file     dummytoolbox.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the NoiseReductionOptionsWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereductionoptionswidget.h"
#include "../noisereduction.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseReductionPlugin;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReductionOptionsWidget::NoiseReductionOptionsWidget(NoiseReduction* toolbox, QWidget* parent)
: QWidget(parent, Qt::Window)
, ui(new Ui::NoiseReductionOptionsWidgetClass)
, m_pNoiseReductionToolbox(toolbox)
, m_enableDisableProjectors(Q_NULLPTR)
, m_pShowFilterOptions(Q_NULLPTR)
, m_pCompSignalMapper(Q_NULLPTR)
{
    this->setWindowTitle("Noise reduction options");

    ui->setupUi(this);

    //Do the connects. Always connect GUI elemts after ui.setpUi has been called
    connect(ui->m_checkBox_activateSphara, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            m_pNoiseReductionToolbox, &NoiseReduction::setSpharaMode);
    connect(ui->m_spinBox_nBaseFctsMag, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &NoiseReductionOptionsWidget::onNBaseFctsChanged);
    connect(ui->m_spinBox_nBaseFctsGrad, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &NoiseReductionOptionsWidget::onNBaseFctsChanged);
    connect(ui->m_comboBox_acquisitionSystem, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
            m_pNoiseReductionToolbox, &NoiseReduction::setAcquisitionSystem);
    connect(ui->m_comboBox_acquisitionSystem, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
            this, &NoiseReductionOptionsWidget::setAcquisitionSystem);
}


//*************************************************************************************************************

NoiseReductionOptionsWidget::~NoiseReductionOptionsWidget()
{
    delete ui;
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::setFiffInfo(const FiffInfo::SPtr pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;

    //Create projectors
    createProjectorGroup();

    //Create compensators
    createCompensatorGroup();
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::setAcquisitionSystem(const QString &sSystem)
{
    ui->m_label_nBaseFctsMag->show();
    ui->m_spinBox_nBaseFctsMag->show();
    ui->m_spinBox_nBaseFctsMag->show();

    ui->m_label_nBaseFctsGrad->show();
    ui->m_spinBox_nBaseFctsGrad->show();
    ui->m_spinBox_nBaseFctsGrad->show();

    if(sSystem == "VectorView") {
        ui->m_label_nBaseFctsMag->setText("Mag");
        ui->m_spinBox_nBaseFctsMag->setMaximum(102);
        ui->m_spinBox_nBaseFctsMag->setValue(102);

        ui->m_label_nBaseFctsGrad->setText("Grad");
        ui->m_spinBox_nBaseFctsGrad->setMaximum(102);
        ui->m_spinBox_nBaseFctsGrad->setValue(102);
    }

    if(sSystem == "BabyMEG") {
        ui->m_label_nBaseFctsMag->setText("Outer layer");
        ui->m_spinBox_nBaseFctsMag->setMaximum(105);
        ui->m_spinBox_nBaseFctsMag->setValue(105);

        ui->m_label_nBaseFctsGrad->setText("Inner layer");
        ui->m_spinBox_nBaseFctsGrad->setMaximum(270);
        ui->m_spinBox_nBaseFctsGrad->setValue(270);
    }

    if(sSystem == "EEG") {
        ui->m_label_nBaseFctsMag->hide();
        ui->m_spinBox_nBaseFctsMag->hide();
        ui->m_spinBox_nBaseFctsMag->hide();

        ui->m_label_nBaseFctsGrad->setText("EEG");
        ui->m_spinBox_nBaseFctsGrad->setMaximum(256);
        ui->m_spinBox_nBaseFctsGrad->setValue(256);
    }
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::filterGroupChanged(QList<QCheckBox*> list)
{
    m_qFilterListCheckBox.clear();

    qDebug() << "list.size(): " << list.size();

    for(int u = 0; u < list.size(); u++) {
        QCheckBox* tempCheckBox = new QCheckBox(list[u]->text());
        tempCheckBox->setChecked(list[u]->isChecked());

        connect(tempCheckBox, &QCheckBox::toggled,
                list[u], &QCheckBox::setChecked);

        if(tempCheckBox->text() == "Activate user designed filter")
            connect(tempCheckBox, &QCheckBox::toggled,
                    this, &NoiseReductionOptionsWidget::onUserFilterToggled);

        connect(list[u], &QCheckBox::toggled,
                tempCheckBox, &QCheckBox::setChecked);

        m_qFilterListCheckBox.append(tempCheckBox);
    }

    //Delete all widgets in the filter layout
    //QGridLayout* topLayout = static_cast<QGridLayout*>(ui->m_groupBox_temporalFiltering->layout());
    //if(!topLayout) {
       QGridLayout* topLayout = new QGridLayout();
    //}

//    QLayoutItem *child;
//    while((child = topLayout->takeAt(0)) != 0) {
//        delete child->widget();
//        delete child;
//    }

    //Add filters
    int u = 0;

    qDebug() << "m_qFilterListCheckBox.size(): " << m_qFilterListCheckBox.size();

    for(u; u < m_qFilterListCheckBox.size(); ++u) {
        topLayout->addWidget(m_qFilterListCheckBox[u], u, 0);
    }

    //Add push button for filter options
    m_pShowFilterOptions = new QPushButton();
//        m_pShowFilterOptions->setText("Open Filter options");
    m_pShowFilterOptions->setText("Filter options");
    m_pShowFilterOptions->setCheckable(false);
    connect(m_pShowFilterOptions, &QPushButton::clicked,
            this, &NoiseReductionOptionsWidget::onShowFilterOptions);

    topLayout->addWidget(m_pShowFilterOptions, u+1, 0);

    //Find Filter tab and add current layout
    ui->m_groupBox_temporalFiltering->setLayout(topLayout);
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::onCheckProjStatusChanged(bool status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if(m_qListProjCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListProjCheckBox[i]->isChecked();
    }

    if(m_enableDisableProjectors) {
        m_enableDisableProjectors->setChecked(bAllActivated);
    }

    emit projSelectionChanged();
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::onEnableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for(int i=0; i<m_qListProjCheckBox.size(); i++)
        m_qListProjCheckBox.at(i)->setChecked(status);

    //Set all projection activation states to status
    for(int i=0; i < m_pFiffInfo->projs.size(); ++i)
        m_pFiffInfo->projs[i].active = status;

    if(m_enableDisableProjectors) {
        m_enableDisableProjectors->setChecked(status);
    }

    emit projSelectionChanged();
}

//*************************************************************************************************************

void NoiseReductionOptionsWidget::onCheckCompStatusChanged(const QString & compName)
{
    //qDebug()<<compName;

    bool currentState = false;

    for(int i = 0; i < m_qListCompCheckBox.size(); ++i)
        if(m_qListCompCheckBox[i]->text() != compName)
            m_qListCompCheckBox[i]->setChecked(false);
        else
            currentState = m_qListCompCheckBox[i]->isChecked();

    if(currentState)
        emit compSelectionChanged(compName.toInt());
    else //If none selected
        emit compSelectionChanged(0);
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::onNBaseFctsChanged()
{
    m_pNoiseReductionToolbox->setSpharaNBaseFcts(ui->m_spinBox_nBaseFctsGrad->value(), ui->m_spinBox_nBaseFctsMag->value());
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::onShowFilterOptions(bool state)
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

void NoiseReductionOptionsWidget::onUserFilterToggled(bool state)
{
    Q_UNUSED(state);
    //qDebug()<<"onUserFilterToggled";
    emit updateConnectedView();
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::createProjectorGroup()
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
                    this, &NoiseReductionOptionsWidget::onCheckProjStatusChanged);

            topLayout->addWidget(checkBox, i, 0); //+2 because we already added two widgets before the first projector check box
        }

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        topLayout->addWidget(line, i+1, 0);

        m_enableDisableProjectors = new QCheckBox("Enable all");
        m_enableDisableProjectors->setChecked(bAllActivated);
        topLayout->addWidget(m_enableDisableProjectors, i+2, 0);
        connect(m_enableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &NoiseReductionOptionsWidget::onEnableDisableAllProj);

        //Find SSP tab and add current layout
        ui->m_groupBox_projectors->setLayout(topLayout);

        //Set default activation to true
        onEnableDisableAllProj(true);
    }
}


//*************************************************************************************************************

void NoiseReductionOptionsWidget::createCompensatorGroup()
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

        connect(this, &NoiseReductionOptionsWidget::compClicked,
                this, &NoiseReductionOptionsWidget::onCheckCompStatusChanged);

        //Find Comp tab and add current layout
        ui->m_groupBox_compensators->setLayout(topLayout);
    }
}


