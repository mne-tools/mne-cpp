//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     noisereductionwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     December, 2014
 * @version  2.1.0
 * @brief    Definition of the NoiseReductionWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "noisereductionwindow.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReductionWindow::NoiseReductionWindow(QWidget* parent)
: QDockWidget(parent)
, ui(new Ui::NoiseReductionWindow)
, m_enableDisableProjectors(nullptr)
, m_pCompSignalMapper(nullptr)
{
    ui->setupUi(this);
}


//*************************************************************************************************************

NoiseReductionWindow::NoiseReductionWindow(QWidget* parent, FiffInfo* pFiffInfo)
: QDockWidget(parent)
, ui(new Ui::NoiseReductionWindow)
, m_pFiffInfo(pFiffInfo)
{
    ui->setupUi(this);

    createProjectorGroup();
    createCompensatorGroup();
}


//*************************************************************************************************************

void NoiseReductionWindow::setFiffInfo(std::shared_ptr<FiffInfo>& pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;

    createProjectorGroup();
    createCompensatorGroup();
}


//*************************************************************************************************************

void NoiseReductionWindow::toggleAllProjectors()
{
    if (!m_pFiffInfo || m_qListProjCheckBox.isEmpty())
        return;

    // Determine new state: if all are on, turn all off; otherwise turn all on
    bool allOn = true;
    for (int i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if (!m_qListProjCheckBox[i]->isChecked()) {
            allOn = false;
            break;
        }
    }
    enableDisableAllProj(!allOn);
}


//*************************************************************************************************************

void NoiseReductionWindow::createProjectorGroup()
{
    if (m_pFiffInfo) {
        if (ui->m_groupBox_projections->layout() != 0)
            this->remove(ui->m_groupBox_projections->layout());

        m_qListProjCheckBox.clear();

        // Projection Selection
        QGridLayout* topLayout = new QGridLayout;

        if (!m_pFiffInfo->projs.empty()) {
            bool bAllActivated = true;

            qint32 i = 0;

            for (; i < std::ssize(m_pFiffInfo->projs); ++i) {
                QCheckBox* checkBox = new QCheckBox(QString::fromStdString(m_pFiffInfo->projs[i].desc));
                checkBox->setChecked(m_pFiffInfo->projs[i].active);

                if (m_pFiffInfo->projs[i].active == false)
                    bAllActivated = false;

                m_qListProjCheckBox.append(checkBox);

                connect(checkBox, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                        this, &NoiseReductionWindow::checkProjStatusChanged);

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

            topLayout->addWidget(line, i + 1, 0);

            m_enableDisableProjectors = new QCheckBox("Enable all");
            m_enableDisableProjectors->setChecked(bAllActivated);
            topLayout->addWidget(m_enableDisableProjectors, i + 2, 0);
            connect(m_enableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                    this, &NoiseReductionWindow::enableDisableAllProj);
        }

        // The group box accepts the new layout only once the old one is gone
        std::unique_ptr<QLayout> pOldLayout(ui->m_groupBox_projections->layout());
        pOldLayout.reset();
        ui->m_groupBox_projections->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void NoiseReductionWindow::createCompensatorGroup()
{
    if (m_pFiffInfo) {
        m_pCompSignalMapper = new QSignalMapper(this);

        if (ui->m_groupBox_compensators->layout() != 0)
            this->remove(ui->m_groupBox_compensators->layout());

        m_qListCompCheckBox.clear();

        // Compensation Selection
        QGridLayout* topLayout = new QGridLayout;

        if (!m_pFiffInfo->comps.empty()) {
            qint32 i = 0;

            for (; i < std::ssize(m_pFiffInfo->comps); ++i) {
                QString numStr;
                QCheckBox* checkBox = new QCheckBox(numStr.setNum(m_pFiffInfo->comps[i].kind));

                m_qListCompCheckBox.append(checkBox);

                connect(checkBox, SIGNAL(clicked()),
                        m_pCompSignalMapper, SLOT(map()));

                m_pCompSignalMapper->setMapping(checkBox, numStr);

                topLayout->addWidget(checkBox, i, 0);
            }

            connect(m_pCompSignalMapper, SIGNAL(mapped(const QString&)),
                    this, SIGNAL(compClicked(const QString&)));

            connect(this, &NoiseReductionWindow::compClicked,
                    this, &NoiseReductionWindow::checkCompStatusChanged);
        }

        // The group box accepts the new layout only once the old one is gone
        std::unique_ptr<QLayout> pOldLayout(ui->m_groupBox_compensators->layout());
        pOldLayout.reset();
        ui->m_groupBox_compensators->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void NoiseReductionWindow::enableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for (int i = 0; i < m_qListProjCheckBox.size(); i++)
        m_qListProjCheckBox.at(i)->setChecked(status);

    //Set all projection activation states to status
    for (int i = 0; i < std::ssize(m_pFiffInfo->projs); ++i)
        m_pFiffInfo->projs[i].active = status;

    m_enableDisableProjectors->setChecked(status);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void NoiseReductionWindow::checkProjStatusChanged(bool status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for (qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if (m_qListProjCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListProjCheckBox[i]->isChecked();
        mne::Log::warning("[NoiseReduction] proj {} checked= {} FiffInfo ptr= {}", i, m_qListProjCheckBox[i]->isChecked(), reinterpret_cast<void*>(m_pFiffInfo.get()));
    }

    m_enableDisableProjectors->setChecked(bAllActivated);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void NoiseReductionWindow::checkCompStatusChanged(const QString& compName)
{
    bool currentState = false;

    for (int i = 0; i < m_qListCompCheckBox.size(); ++i)
        if (m_qListCompCheckBox[i]->text() != compName)
            m_qListCompCheckBox[i]->setChecked(false);
        else
            currentState = m_qListCompCheckBox[i]->isChecked();

    if (currentState)
        emit compSelectionChanged(compName.toInt());
    else //If none selected
        emit compSelectionChanged(0);
}


//*************************************************************************************************************

void NoiseReductionWindow::remove(QLayout* layout)
{
    while (std::unique_ptr<QLayoutItem> pChild{layout->takeAt(0)}) {
        if (QLayout* pChildLayout = pChild->layout()) {
            remove(pChildLayout);
        } else if (QWidget* pWidget = pChild->widget()) {
            pWidget->deleteLater();
        }
    }
}
