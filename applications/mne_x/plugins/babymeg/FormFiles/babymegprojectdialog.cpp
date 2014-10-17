//=============================================================================================================
/**
* @file     babymegprojectdialog.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BabyMEGProjectDialog class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegprojectdialog.h"
#include "ui_babymegprojectdialog.h"

#include "../babymeg.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGProjectDialog::BabyMEGProjectDialog(BabyMEG* p_pBabyMEG, QWidget *parent)
: m_pBabyMEG(p_pBabyMEG)
, QDialog(parent)
, ui(new Ui::BabyMEGProjectDialog)
{
    ui->setupUi(this);

    //BabyMEGData Path
    m_sBabyMEGDataPath = QDir::homePath() + "/BabyMEGData";
    if(!QDir(m_sBabyMEGDataPath).exists())
        QDir().mkdir(m_sBabyMEGDataPath);

    //Test Project
    if(!QDir(m_sBabyMEGDataPath+"/TestProject").exists())
        QDir().mkdir(m_sBabyMEGDataPath+"/TestProject");
    QSettings settings;
    m_sCurrentProject = settings.value(QString("Plugin/%1/currentProject").arg(p_pBabyMEG->getName()), "TestProject").toString();
    scanForProjects();

    //Test Subject
    if(!QDir(m_sBabyMEGDataPath+"/TestProject/TestSubject").exists())
        QDir().mkdir(m_sBabyMEGDataPath+"/TestProject/TestSubject");
    m_sCurrentSubject = settings.value(QString("Plugin/%1/currentSubject").arg(p_pBabyMEG->getName()), "TestSubject").toString();
    scanForSubjects();

    ui->m_qLineEditFileName->setReadOnly(true);

    updateFileName();
}


//*************************************************************************************************************

BabyMEGProjectDialog::~BabyMEGProjectDialog()
{
    delete ui;
}


//*************************************************************************************************************

QString BabyMEGProjectDialog::getFilePath() const
{
    QString sFilePath = m_sBabyMEGDataPath + "/" + m_sCurrentProject + "/" + m_sCurrentSubject;

    return sFilePath;
}


//*************************************************************************************************************

void BabyMEGProjectDialog::scanForProjects()
{
    //clear
    ui->m_qComboBox_ProjectSelection->clear();
    m_sListProjects.clear();

    QDir t_qDirData(m_sBabyMEGDataPath);

    QFileInfoList t_qFileInfoList = t_qDirData.entryInfoList();
    QFileInfoList::const_iterator it;
    for (it = t_qFileInfoList.constBegin(); it != t_qFileInfoList.constEnd(); ++it)
        if(it->isDir() && it->fileName() != "." && it->fileName() != "..")
            m_sListProjects.append(it->fileName());

    ui->m_qComboBox_ProjectSelection->insertItems(0,m_sListProjects);
    ui->m_qComboBox_ProjectSelection->setCurrentIndex(ui->m_qComboBox_ProjectSelection->findText(m_sCurrentProject));
}


//*************************************************************************************************************

void BabyMEGProjectDialog::scanForSubjects()
{
    //clear
    ui->m_qComboBox_SubjectSelection->clear();
    m_sListSubjects.clear();

    QDir t_qDirProject(m_sBabyMEGDataPath+"/"+m_sCurrentProject);

    QFileInfoList t_qFileInfoList = t_qDirProject.entryInfoList();
    QFileInfoList::const_iterator it;
    for (it = t_qFileInfoList.constBegin(); it != t_qFileInfoList.constEnd(); ++it)
        if(it->isDir() && it->fileName() != "." && it->fileName() != "..")
            m_sListSubjects.append(it->fileName());

    ui->m_qComboBox_SubjectSelection->insertItems(0,m_sListSubjects);
    ui->m_qComboBox_SubjectSelection->setCurrentIndex(ui->m_qComboBox_SubjectSelection->findText(m_sCurrentSubject));
}


//*************************************************************************************************************

void BabyMEGProjectDialog::updateFileName()
{
    ui->m_qLineEditFileName->setText(getFilePath() + "/<YYMMDD_HMS>_" + m_sCurrentSubject + "_raw.fif");
}
