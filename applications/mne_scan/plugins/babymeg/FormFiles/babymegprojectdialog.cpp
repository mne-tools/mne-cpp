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
* @brief    BabyMEGProjectDialog class definition.
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
#include <QInputDialog>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGProjectDialog::BabyMEGProjectDialog(BabyMEG* p_pBabyMEG, QWidget *parent)
: m_pBabyMEG(p_pBabyMEG)
, QDialog(parent)
, ui(new Ui::BabyMEGProjectDialog)
, m_iRecordingTime(5*60*1000)
{
    ui->setupUi(this);

    scanForProjects();
    scanForSubjects();

    connect(ui->m_qComboBox_ProjectSelection,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
                this,&BabyMEGProjectDialog::selectNewProject);

    connect(ui->m_qComboBox_SubjectSelection,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
                this,&BabyMEGProjectDialog::selectNewSubject);

    connect(ui->m_qLineEditParadigm,&QLineEdit::textChanged,
                this,&BabyMEGProjectDialog::paradigmChanged);

    connect(ui->m_qPushButtonNewProject,&QPushButton::clicked,
                this,&BabyMEGProjectDialog::addProject);

    connect(ui->m_qPushButtonNewSubject,&QPushButton::clicked,
                this,&BabyMEGProjectDialog::addSubject);

    connect(ui->m_qPushButtonDeleteProject,&QPushButton::clicked,
                this,&BabyMEGProjectDialog::deleteProject);

    connect(ui->m_qPushButtonDeleteSubject,&QPushButton::clicked,
                this,&BabyMEGProjectDialog::deleteSubject);

    connect(ui->m_spinBox_hours, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&BabyMEGProjectDialog::onTimeChanged);

    connect(ui->m_spinBox_min, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&BabyMEGProjectDialog::onTimeChanged);

    connect(ui->m_spinBox_sec, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&BabyMEGProjectDialog::onTimeChanged);

    connect(ui->m_checkBox_useRecordingTimer,&QCheckBox::toggled,
                this,&BabyMEGProjectDialog::onRecordingTimerStateChanged);

    ui->m_qLineEditFileName->setReadOnly(true);

    updateFileName();

    //Hide remaining time
    ui->m_label_RemainingTime->hide();
    ui->m_label_timeToGo->hide();

//    QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
//                                              tr("User name:"), QLineEdit::Normal,
//                                              QDir::home().dirName(), &ok);

    //Hide delete buttons
    ui->m_qPushButtonDeleteProject->hide();
    ui->m_qPushButtonDeleteSubject->hide();
}


//*************************************************************************************************************

BabyMEGProjectDialog::~BabyMEGProjectDialog()
{
    delete ui;
}


//*************************************************************************************************************

void BabyMEGProjectDialog::setRecordingElapsedTime(int mSecsElapsed)
{
    QTime remainingTime(0,0,0,0);

    QTime remainingTimeFinal = remainingTime.addMSecs(m_iRecordingTime-mSecsElapsed);

    ui->m_label_timeToGo->setText(remainingTimeFinal.toString());

    QTime passedTime(0,0,0,0);

    QTime passedTimeFinal = passedTime.addMSecs(mSecsElapsed);

    ui->m_label_timePassed->setText(passedTimeFinal.toString());
}


//*************************************************************************************************************

void BabyMEGProjectDialog::deleteSubject()
{
    QMessageBox msgBox;
    msgBox.setText(QString("Deleting subject data '%1'.").arg(ui->m_qComboBox_SubjectSelection->currentText()));
    msgBox.setInformativeText("You are about to delete a subject. Do you want to delete all data corresponding to this subject?");
    msgBox.setIcon(QMessageBox::Warning);
    QPushButton *keepData = msgBox.addButton(tr("Keep data"), QMessageBox::ActionRole);
    QPushButton *deleteData = msgBox.addButton(tr("Delete data"), QMessageBox::ActionRole);

    msgBox.exec();

    if(msgBox.clickedButton() == keepData)
        return;

    if(msgBox.clickedButton() == deleteData) {
        QMessageBox msgBox;
        msgBox.setText(QString("Deleting subject data '%1'.").arg(ui->m_qComboBox_SubjectSelection->currentText()));
        msgBox.setInformativeText("Do really want to delete the data permantley? The deleted data cannot be recovered!");
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.setIcon(QMessageBox::Critical);
        int ret = msgBox.exec();

        if(ret == QMessageBox::No)
            return;

        QString dirName = m_pBabyMEG->getDataPath() + "/" + ui->m_qComboBox_ProjectSelection->currentText() + "/" + ui->m_qComboBox_SubjectSelection->currentText();

        QDir dir(dirName);

        bool result = dir.removeRecursively();

        if(!result)
            qDebug()<<"Could not remove all data from the subject folder!";
        else
            ui->m_qComboBox_SubjectSelection->removeItem(ui->m_qComboBox_SubjectSelection->currentIndex());
    }
}


//*************************************************************************************************************

void BabyMEGProjectDialog::deleteProject()
{
    QMessageBox msgBox;
    msgBox.setText(QString("Deleting project data '%1'.").arg(ui->m_qComboBox_ProjectSelection->currentText()));
    msgBox.setInformativeText("You are about to delete a project. Do you want to delete all data corresponding to this project?");
    msgBox.setIcon(QMessageBox::Warning);
    QPushButton *keepData = msgBox.addButton(tr("Keep data"), QMessageBox::ActionRole);
    QPushButton *deleteData = msgBox.addButton(tr("Delete data"), QMessageBox::ActionRole);

    msgBox.exec();

    if(msgBox.clickedButton() == keepData)
        return;

    if(msgBox.clickedButton() == deleteData) {
        QMessageBox msgBox;
        msgBox.setText(QString("Deleting project data '%1'.").arg(ui->m_qComboBox_ProjectSelection->currentText()));
        msgBox.setInformativeText("Do really want to delete the data permantley? All subject data in this project will be lost! The deleted data cannot be recovered!");
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.setIcon(QMessageBox::Critical);
        int ret = msgBox.exec();

        if(ret == QMessageBox::No)
            return;

        QString dirName = m_pBabyMEG->getDataPath() + "/" + ui->m_qComboBox_ProjectSelection->currentText();

        QDir dir(dirName);

        bool result = dir.removeRecursively();

        if(!result)
            qDebug()<<"Could not remove all data from the project folder!";
        else
            ui->m_qComboBox_ProjectSelection->removeItem(ui->m_qComboBox_ProjectSelection->currentIndex());
    }
}


//*************************************************************************************************************

void BabyMEGProjectDialog::addProject()
{
    bool ok;
    QString sProject = QInputDialog::getText(this, tr("Add new Project"),
                                              tr("Add new Project:"), QLineEdit::Normal,
                                              tr("NewProject"), &ok);
    if (ok && !sProject.isEmpty())
    {
        if(!QDir(m_pBabyMEG->m_sBabyMEGDataPath+"/" + sProject).exists())
            QDir().mkdir(m_pBabyMEG->m_sBabyMEGDataPath+"/"+sProject);

        m_pBabyMEG->m_sCurrentProject = sProject;

        scanForProjects();
    }
}


//*************************************************************************************************************

void BabyMEGProjectDialog::addSubject()
{
    bool ok;
    QString sSubject = QInputDialog::getText(this, tr("Add new Subject"),
                                              tr("Add new Subject:"), QLineEdit::Normal,
                                              tr("NewSubject"), &ok);

    if (ok && !sSubject.isEmpty())
    {
        if(!QDir(m_pBabyMEG->m_sBabyMEGDataPath + "/" + m_pBabyMEG->m_sCurrentProject + "/" + sSubject).exists())
            QDir().mkdir(m_pBabyMEG->m_sBabyMEGDataPath + "/" + m_pBabyMEG->m_sCurrentProject + "/" + sSubject);

        m_pBabyMEG->m_sCurrentSubject = sSubject;

        scanForSubjects();
    }
}


//*************************************************************************************************************

void BabyMEGProjectDialog::paradigmChanged(const QString &newParadigm)
{
    m_pBabyMEG->m_sCurrentParadigm = newParadigm;
    updateFileName();
}


//*************************************************************************************************************

void BabyMEGProjectDialog::scanForProjects()
{
    //clear
    ui->m_qComboBox_ProjectSelection->clear();
    m_sListProjects.clear();

    QDir t_qDirData(m_pBabyMEG->m_sBabyMEGDataPath);

    QFileInfoList t_qFileInfoList = t_qDirData.entryInfoList();
    QFileInfoList::const_iterator it;
    for (it = t_qFileInfoList.constBegin(); it != t_qFileInfoList.constEnd(); ++it)
        if(it->isDir() && it->fileName() != "." && it->fileName() != "..")
            m_sListProjects.append(it->fileName());

    ui->m_qComboBox_ProjectSelection->insertItems(0,m_sListProjects);
    ui->m_qComboBox_ProjectSelection->setCurrentIndex(ui->m_qComboBox_ProjectSelection->findText(m_pBabyMEG->m_sCurrentProject));
}


//*************************************************************************************************************

void BabyMEGProjectDialog::scanForSubjects()
{
    //clear
    ui->m_qComboBox_SubjectSelection->clear();
    m_sListSubjects.clear();

    QDir t_qDirProject(m_pBabyMEG->m_sBabyMEGDataPath+"/"+m_pBabyMEG->m_sCurrentProject);

    QFileInfoList t_qFileInfoList = t_qDirProject.entryInfoList();
    QFileInfoList::const_iterator it;
    for (it = t_qFileInfoList.constBegin(); it != t_qFileInfoList.constEnd(); ++it)
        if(it->isDir() && it->fileName() != "." && it->fileName() != "..")
            m_sListSubjects.append(it->fileName());

    ui->m_qComboBox_SubjectSelection->insertItems(0,m_sListSubjects);

    qint32 idx = ui->m_qComboBox_SubjectSelection->findText(m_pBabyMEG->m_sCurrentSubject);
    if(idx >= 0)
        ui->m_qComboBox_SubjectSelection->setCurrentIndex(idx);
    else
    {
        ui->m_qComboBox_SubjectSelection->setCurrentIndex(0);
        selectNewSubject(ui->m_qComboBox_SubjectSelection->itemText(0));
    }
}


//*************************************************************************************************************

void BabyMEGProjectDialog::selectNewProject(const QString &newProject)
{
    m_pBabyMEG->m_sCurrentProject = newProject;

    scanForSubjects();
    updateFileName();
}


//*************************************************************************************************************

void BabyMEGProjectDialog::selectNewSubject(const QString &newSubject)
{
    m_pBabyMEG->m_sCurrentSubject = newSubject;

    updateFileName();
}


//*************************************************************************************************************

void BabyMEGProjectDialog::updateFileName()
{
    ui->m_qLineEditFileName->setText(m_pBabyMEG->getFilePath());
}


//*************************************************************************************************************

void BabyMEGProjectDialog::onTimeChanged()
{
    m_iRecordingTime = (ui->m_spinBox_hours->value()*60*60)+(ui->m_spinBox_min->value()*60)+(ui->m_spinBox_sec->value());

    m_iRecordingTime*=1000;

    QTime remainingTime(0,0,0,0);

    QTime remainingTimeFinal = remainingTime.addMSecs(m_iRecordingTime);

    ui->m_label_timeToGo->setText(remainingTimeFinal.toString());

    emit timerChanged(m_iRecordingTime);
}


//*************************************************************************************************************

void BabyMEGProjectDialog::onRecordingTimerStateChanged(bool state)
{
    emit recordingTimerStateChanged(state);
}

