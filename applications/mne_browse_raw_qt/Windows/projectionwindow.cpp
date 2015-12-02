//=============================================================================================================
/**
* @file     projectionwindow.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2014
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
* @brief    Contains the implementation of the ProjectionWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectionwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectionWindow::ProjectionWindow(QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::ProjectionWindow)
, m_pProjectionModel(new ProjectionModel(this))
{
    ui->setupUi(this);

    initTableViewWidgets();
}


//*************************************************************************************************************

ProjectionWindow::ProjectionWindow(QWidget *parent, QFile& qFile)
: QDockWidget(parent)
, ui(new Ui::ProjectionWindow)
, m_pProjectionModel(new ProjectionModel(this, qFile))
{
    ui->setupUi(this);

    initTableViewWidgets();
}


//*************************************************************************************************************

ProjectionWindow::ProjectionWindow(QWidget *parent, QList<FiffProj>& dataProjs)
: QDockWidget(parent)
, ui(new Ui::ProjectionWindow)
, m_pProjectionModel(new ProjectionModel(this, dataProjs))
{
    ui->setupUi(this);

    initTableViewWidgets();
}


//*************************************************************************************************************

ProjectionWindow::ProjectionWindow(QWidget *parent, FiffInfo::SPtr pFiffInfo)
: QDockWidget(parent)
, ui(new Ui::ProjectionWindow)
, m_pFiffInfo(pFiffInfo)
{
    ui->setupUi(this);

    createProjectorGroup();
    createCompensatorGroup();
}


//*************************************************************************************************************

void ProjectionWindow::setFiffInfo(FiffInfo::SPtr pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;

    createProjectorGroup();
    createCompensatorGroup();
}


//*************************************************************************************************************

ProjectionModel* ProjectionWindow::getDataModel()
{
    return m_pProjectionModel;
}


//*************************************************************************************************************

void ProjectionWindow::initTableViewWidgets()
{
//    //Set model
//    ui->m_tableView_availableProjections->setModel(m_pProjectionModel);

//    connect(m_pProjectionModel, &ProjectionModel::dataChanged,
//            ui->m_tableView_availableProjections, &QTableView::resizeColumnsToContents);

//    //Hide data column in view
//    ui->m_tableView_availableProjections->setColumnHidden(3, true);

//    ui->m_tableView_availableProjections->hide();
}


//*************************************************************************************************************

void ProjectionWindow::createProjectorGroup()
{
    if(m_pFiffInfo)
    {
        if(ui->m_groupBox_projections->layout() != 0)
            this->remove(ui->m_groupBox_projections->layout());

        // Projection Selection
        QGridLayout *topLayout = new QGridLayout;

        if(!m_pFiffInfo->projs.isEmpty())
        {
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
                        this, &ProjectionWindow::checkProjStatusChanged);

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
                this, &ProjectionWindow::enableDisableAllProj);

            emit projSelectionChanged();
        }

        delete ui->m_groupBox_projections->layout();
        ui->m_groupBox_projections->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void ProjectionWindow::createCompensatorGroup()
{
    if(m_pFiffInfo)
    {
        m_pCompSignalMapper = new QSignalMapper(this);

        if(ui->m_groupBox_compensators->layout() != 0)
            this->remove(ui->m_groupBox_compensators->layout());

        // Compensation Selection
        QGridLayout *topLayout = new QGridLayout;

        if(!m_pFiffInfo->comps.isEmpty())
        {
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

            connect(this, &ProjectionWindow::compClicked,
                    this, &ProjectionWindow::checkCompStatusChanged);
        }

        delete ui->m_groupBox_compensators->layout();
        ui->m_groupBox_compensators->setLayout(topLayout);
    }
}


//*************************************************************************************************************

void ProjectionWindow::enableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for(int i=0; i<m_qListProjCheckBox.size(); i++)
        m_qListProjCheckBox.at(i)->setChecked(status);

    //Set all projection activation states to status
    for(int i=0; i < m_pFiffInfo->projs.size(); ++i)
        m_pFiffInfo->projs[i].active = status;

    m_enableDisableProjectors->setChecked(status);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void ProjectionWindow::checkProjStatusChanged(bool status)
{
    Q_UNUSED(status)

    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if(m_qListProjCheckBox[i]->isChecked() == false)
            bAllActivated = false;

        this->m_pFiffInfo->projs[i].active = m_qListProjCheckBox[i]->isChecked();
    }

    m_enableDisableProjectors->setChecked(bAllActivated);

    emit projSelectionChanged();
}


//*************************************************************************************************************

void ProjectionWindow::checkCompStatusChanged(const QString & compName)
{
    qDebug()<<compName;

    bool currentState;

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

void ProjectionWindow::remove(QLayout* layout)
{
    QLayoutItem* child;
    while(layout->count()!=0)
    {
        child = layout->takeAt(0);
        if(child->layout() != 0)
        {
            remove(child->layout());
        }
        else if(child->widget() != 0)
        {
            delete child->widget();
        }

        delete child;
    }
}
