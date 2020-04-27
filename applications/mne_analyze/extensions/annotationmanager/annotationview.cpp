//=============================================================================================================
/**
 * @file     annotationview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the AnnotationView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationview.h"
#include "ui_annotationview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QMap>
#include <QToolBar>
#include <QColorDialog>
#include <QFileDialog>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationView::AnnotationView()
: ui(new Ui::EventWindowDockWidget)
, m_iCheckState(0)
, m_iLastSampClicked(0)
, m_pAnnModel(Q_NULLPTR)
, m_pColordialog(new QColorDialog(this))
{
    ui->setupUi(this);

    ui->m_comboBox_filterTypes->addItem("All");
    ui->m_comboBox_filterTypes->addItem("0");
    ui->m_comboBox_filterTypes->setCurrentText("All");

    onDataChanged();
}

//=============================================================================================================

void AnnotationView::initMSVCSettings()
{
    //Model
    ui->m_tableView_eventTableView->setModel(m_pAnnModel.data());
    connect(m_pAnnModel.data(),&ANSHAREDLIB::AnnotationModel::dataChanged,
            this, &AnnotationView::onDataChanged, Qt::UniqueConnection);

    //Delegate
    m_pAnnDelegate = QSharedPointer<AnnotationDelegate>(new AnnotationDelegate(this));
    ui->m_tableView_eventTableView->setItemDelegate(m_pAnnDelegate.data());

    ui->m_tableView_eventTableView->resizeColumnsToContents();
    ui->m_tableView_eventTableView->adjustSize();
}

//=============================================================================================================

void AnnotationView::initGUIFunctionality()
{

    //'Activate annotations' checkbox
    connect(ui->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &AnnotationView::onActiveEventsChecked, Qt::UniqueConnection);

    //'Show selected annotation' checkbox
    connect(ui->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &AnnotationView::onSelectedEventsChecked, Qt::UniqueConnection);
    connect(ui->m_tableView_eventTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationView::onCurrentSelectedChanged, Qt::UniqueConnection);

    //Annotation types combo box

    //ui->m_comboBox_filterTypes->addItems(m_pAnnModel->getEventTypeList());

    connect(ui->m_comboBox_filterTypes, &QComboBox::currentTextChanged,
            m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::setEventFilterType, Qt::UniqueConnection);
    connect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::updateEventTypes,
            this, &AnnotationView::updateComboBox, Qt::UniqueConnection);

    //'Remove annotations' button
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(false);
    QAction* removeEvent = new QAction("Remove", this);
    removeEvent->setStatusTip(tr("Remove an annotation from the list"));
    toolBar->addAction(removeEvent);
    connect(removeEvent, &QAction::triggered,
            this, &AnnotationView::removeAnnotationfromModel, Qt::UniqueConnection);
    ui->m_gridLayout_Main->addWidget(toolBar,1,1,1,1);

    //Add type button
    connect(ui->m_pushButton_addEventType, &QPushButton::clicked,
            this, &AnnotationView::addNewAnnotationType, Qt::UniqueConnection);

    connect(ui->m_pushButtonSave, &QPushButton::clicked,
            this, &AnnotationView::onSaveButton, Qt::UniqueConnection);
}

//=============================================================================================================

void AnnotationView::onActiveEventsChecked(int iCheckBoxState)
{
    qDebug() << "onActiveEventsChecked" << iCheckBoxState;
    m_iCheckState = iCheckBoxState;
    emit activeEventsChecked(m_iCheckState);
}

//=============================================================================================================

void AnnotationView::updateComboBox(const QString &currentAnnotationType)
{
    ui->m_comboBox_filterTypes->clear();
    ui->m_comboBox_filterTypes->addItem("All");
    ui->m_comboBox_filterTypes->addItems(m_pAnnModel->getEventTypeList());
//    if(m_pAnnModel->getEventTypeList().contains(currentAnnotationType))
//        ui->m_comboBox_filterTypes->setCurrentText(currentAnnotationType);

    m_pAnnModel->setLastType(currentAnnotationType.toInt());
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::addAnnotationToModel(const int iSample)
{
    qDebug() << "AnnotationView::addAnnotationToModel -- Here";
    m_iLastSampClicked = iSample;
    m_pAnnModel->setSamplePos(m_iLastSampClicked);
    m_pAnnModel->insertRow(0, QModelIndex());
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::setModel(QSharedPointer<ANSHAREDLIB::AnnotationModel> pAnnModel)
{
    m_pAnnModel = pAnnModel;

    initMSVCSettings();
    initGUIFunctionality();
    onDataChanged();
}

//=============================================================================================================

void AnnotationView::onDataChanged()
{
    qDebug() << "AnnotationView::onDataChanged";
    ui->m_tableView_eventTableView->viewport()->update();
    ui->m_tableView_eventTableView->viewport()->repaint();
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::passFiffParams(int iFirst,int iLast,float fFreq)
{
    m_pAnnModel->setFirstLastSample(iFirst, iLast);
    m_pAnnModel->setSampleFreq(fFreq);
}

//=============================================================================================================

void AnnotationView::removeAnnotationfromModel()
{
    QModelIndexList indexList = ui->m_tableView_eventTableView->selectionModel()->selectedIndexes();
    for(int i = 0; i<indexList.size(); i++)
        m_pAnnModel->removeRow(indexList.at(i).row() /*- i*/);
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::addNewAnnotationType()
{
    m_pAnnModel->addNewAnnotationType(QString().number(ui->m_spinBox_addEventType->value()), m_pColordialog->getColor(Qt::black, this));
    //m_pAnnModel->setEventFilterType(QString().number(ui->m_spinBox_addEventType->value()));
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::onSelectedEventsChecked(int iCheckBoxState)
{
    m_iCheckSelectedState = iCheckBoxState;
    m_pAnnModel->setShowSelected(m_iCheckSelectedState);
    qDebug() << "AnnotationView::onSelectedEventsChecked -- not implemented yet";
    qDebug() << ui->m_tableView_eventTableView->selectionModel()->currentIndex().row();
    //ui->m_tableView_eventTableView->selectionModel()->sle
    //m_pAnnModel
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationView::disconnectFromModel()
{
    disconnect(m_pAnnModel.data(),&ANSHAREDLIB::AnnotationModel::dataChanged,
            this, &AnnotationView::onDataChanged);
    disconnect(ui->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &AnnotationView::onActiveEventsChecked);
    disconnect(ui->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &AnnotationView::onSelectedEventsChecked);
    disconnect(ui->m_comboBox_filterTypes, &QComboBox::currentTextChanged,
            m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::setEventFilterType);
    disconnect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::updateEventTypes,
            this, &AnnotationView::updateComboBox);
    disconnect(ui->m_pushButton_addEventType, &QPushButton::clicked,
            this, &AnnotationView::addNewAnnotationType);

}

//=============================================================================================================

void AnnotationView::onCurrentSelectedChanged()
{
    qDebug() << "AnnotationView::onCurrentSelectedChanged";
    qDebug() << ui->m_tableView_eventTableView->selectionModel()->currentIndex().row();
    m_pAnnModel->setSelectedAnn(ui->m_tableView_eventTableView->selectionModel()->currentIndex().row());
}

//=============================================================================================================

void AnnotationView::onSaveButton()
{
#ifdef WASMBUILD
    m_pAnnModel->saveToFile("");
#else
    QString fileName = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                    tr("Save Annotations"), "",
                                                    tr("Event file (*.eve);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }
    qDebug() << "AnnotationView::onSaveButton";

    m_pAnnModel->saveToFile(fileName);
#endif
}
