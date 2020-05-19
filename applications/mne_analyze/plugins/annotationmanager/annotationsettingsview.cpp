//=============================================================================================================
/**
 * @file     annotationsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Definition of the AnnotationSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationsettingsview.h"
#include "ui_annotationsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QMap>
#include <QToolBar>
#include <QColorDialog>
#include <QFileDialog>
#include <QKeyEvent>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationSettingsView::AnnotationSettingsView()
: m_pUi(new Ui::EventWindowDockWidget)
, m_iCheckState(0)
, m_iLastSampClicked(0)
, m_pAnnModel(Q_NULLPTR)
, m_pColordialog(new QColorDialog(this))
{
    m_pUi->setupUi(this);

    m_pUi->m_comboBox_filterTypes->addItem("All");
    m_pUi->m_comboBox_filterTypes->addItem("0");
    m_pUi->m_comboBox_filterTypes->setCurrentText("All");

    onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::initMSVCSettings()
{
    //Model
    m_pUi->m_tableView_eventTableView->setModel(m_pAnnModel.data());
    connect(m_pAnnModel.data(),&ANSHAREDLIB::AnnotationModel::dataChanged,
            this, &AnnotationSettingsView::onDataChanged, Qt::UniqueConnection);

    //Delegate
    m_pAnnDelegate = QSharedPointer<AnnotationDelegate>(new AnnotationDelegate(this));
    m_pUi->m_tableView_eventTableView->setItemDelegate(m_pAnnDelegate.data());

    connect(m_pAnnDelegate.data(), &AnnotationDelegate::sampleValueChanged,
            this, &AnnotationSettingsView::realTimeDataSample, Qt::UniqueConnection);

    connect(m_pAnnDelegate.data(), &AnnotationDelegate::timeValueChanged,
            this, &AnnotationSettingsView::realTimeDataTime, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->resizeColumnsToContents();
    m_pUi->m_tableView_eventTableView->adjustSize();
    m_pUi->m_tableView_eventTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//=============================================================================================================

void AnnotationSettingsView::initGUIFunctionality()
{

    //'Activate annotations' checkbox
    connect(m_pUi->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onActiveEventsChecked, Qt::UniqueConnection);

    //'Show selected annotation' checkbox
    connect(m_pUi->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onSelectedEventsChecked, Qt::UniqueConnection);
    connect(m_pUi->m_tableView_eventTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationSettingsView::onCurrentSelectedChanged, Qt::UniqueConnection);

    //Annotation types combo box

    //m_pUi->m_comboBox_filterTypes->addItems(m_pAnnModel->getEventTypeList());

    connect(m_pUi->m_comboBox_filterTypes, &QComboBox::currentTextChanged,
            m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::setEventFilterType, Qt::UniqueConnection);
    connect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::updateEventTypes,
            this, &AnnotationSettingsView::updateComboBox, Qt::UniqueConnection);

    //For some reason the GUI breakes without this
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(false);
    m_pUi->m_gridLayout_Main->addWidget(toolBar,0,0,1,1);

    //Add type button
    connect(m_pUi->m_pushButton_addEventType, &QPushButton::clicked,
            this, &AnnotationSettingsView::addNewAnnotationType, Qt::UniqueConnection);

    //Save button
    connect(m_pUi->m_pushButtonSave, &QPushButton::clicked,
            this, &AnnotationSettingsView::onSaveButton, Qt::UniqueConnection);

    //Delete button
    connect(m_pUi->m_pushButtonDelete, &QPushButton::clicked,
            this, &AnnotationSettingsView::removeAnnotationfromModel, Qt::UniqueConnection);
}

//=============================================================================================================

void AnnotationSettingsView::onActiveEventsChecked(int iCheckBoxState)
{
    //qDebug() << "onActiveEventsChecked" << iCheckBoxState;
    m_iCheckState = iCheckBoxState;
    emit activeEventsChecked(m_iCheckState);
}

//=============================================================================================================

void AnnotationSettingsView::updateComboBox(const QString &currentAnnotationType)
{
    m_pUi->m_comboBox_filterTypes->clear();
    m_pUi->m_comboBox_filterTypes->addItem("All");
    m_pUi->m_comboBox_filterTypes->addItems(m_pAnnModel->getEventTypeList());
//    if(m_pAnnModel->getEventTypeList().contains(currentAnnotationType))
//        m_pUi->m_comboBox_filterTypes->setCurrentText(currentAnnotationType);

    m_pAnnModel->setLastType(currentAnnotationType.toInt());
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::addAnnotationToModel(const int iSample)
{
    //qDebug() << "AnnotationSettingsView::addAnnotationToModel -- Here";
    m_iLastSampClicked = iSample;
    m_pAnnModel->setSamplePos(m_iLastSampClicked);
    m_pAnnModel->insertRow(0, QModelIndex());
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::setModel(QSharedPointer<ANSHAREDLIB::AnnotationModel> pAnnModel)
{
    m_pAnnModel = pAnnModel;

    initMSVCSettings();
    initGUIFunctionality();
    onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::onDataChanged()
{
    //qDebug() << "AnnotationSettingsView::onDataChanged";
    m_pUi->m_tableView_eventTableView->viewport()->update();
    m_pUi->m_tableView_eventTableView->viewport()->repaint();
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::passFiffParams(int iFirst,int iLast,float fFreq)
{
    m_pAnnModel->setFirstLastSample(iFirst, iLast);
    m_pAnnModel->setSampleFreq(fFreq);
}

//=============================================================================================================

void AnnotationSettingsView::removeAnnotationfromModel()
{
    m_pUi->m_tableView_eventTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    //QModelIndexList indexList = m_pUi->m_tableView_eventTableView->selectionModel()->selectedIndexes();
    QModelIndexList indexList = m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows();

    for(int i = 0; i<indexList.size(); i++) {
        //qDebug() << "Index List" << indexList.at(i).row();
        //qDebug() << "With Modifier" << indexList.at(i).row() - i;
        m_pAnnModel->removeRow(indexList.at(i).row() - i);
    }
    emit triggerRedraw();
    m_pUi->m_tableView_eventTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

}

//=============================================================================================================

void AnnotationSettingsView::addNewAnnotationType()
{
    m_pAnnModel->addNewAnnotationType(QString().number(m_pUi->m_spinBox_addEventType->value()), m_pColordialog->getColor(Qt::black, this));
    //m_pAnnModel->setEventFilterType(QString().number(m_pUi->m_spinBox_addEventType->value()));
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::onSelectedEventsChecked(int iCheckBoxState)
{
    m_iCheckSelectedState = iCheckBoxState;
    m_pAnnModel->setShowSelected(m_iCheckSelectedState);
    //qDebug() << m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row();
    //m_pUi->m_tableView_eventTableView->selectionModel()->sle
    //m_pAnnModel
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::disconnectFromModel()
{
    disconnect(m_pAnnModel.data(),&ANSHAREDLIB::AnnotationModel::dataChanged,
            this, &AnnotationSettingsView::onDataChanged);
    disconnect(m_pUi->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onActiveEventsChecked);
    disconnect(m_pUi->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onSelectedEventsChecked);
    disconnect(m_pUi->m_comboBox_filterTypes, &QComboBox::currentTextChanged,
            m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::setEventFilterType);
    disconnect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::updateEventTypes,
            this, &AnnotationSettingsView::updateComboBox);
    disconnect(m_pUi->m_pushButton_addEventType, &QPushButton::clicked,
            this, &AnnotationSettingsView::addNewAnnotationType);

}

//=============================================================================================================

void AnnotationSettingsView::onCurrentSelectedChanged()
{
    m_pAnnModel->clearSelected();
    //qDebug() << "AnnotationSettingsView::onCurrentSelectedChanged";
    //qDebug() << m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row();
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());
//    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView
//    qDebug() << m_pUi->m_tableView_eventTableView->selectionModel()->selectedIndexes();

    for (int i = 0;  i < m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().size(); i++) {
//        qDebug() << m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().at(i).row();
        m_pAnnModel->appendSelected(m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().at(i).row());
    }
}

//=============================================================================================================

void AnnotationSettingsView::onSaveButton()
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
    qInfo() << "AnnotationSettingsView::onSaveButton";

    m_pAnnModel->saveToFile(fileName);
#endif
}

//=============================================================================================================

void AnnotationSettingsView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Delete) {
        //qDebug() << "[AnnotationSettingsView::keyPressEvent] -- Qt::Key_Delete";
        this->removeAnnotationfromModel();
    }
    if(event->key() == Qt::Key_J) {
        emit jumpToSelected();
    }
}

//=============================================================================================================

void AnnotationSettingsView::realTimeDataSample(int iValue)
{
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());
    //qDebug() << "AnnotationSettingsView::realTimeData --" << iValue;
    m_pAnnModel->updateFilteredSample(iValue);
    this->onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::realTimeDataTime(double dValue)
{
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());
    //qDebug() << "AnnotationSettingsView::realTimeDataTime" << dValue;
    dValue *= m_pAnnModel->getFreq();
    //qDebug() << "AnnotationSettingsView::realTimeDataTime" << dValue;
    int t_iSample = static_cast<int>(dValue);
    m_pAnnModel->updateFilteredSample(t_iSample);
    this->onDataChanged();
}
