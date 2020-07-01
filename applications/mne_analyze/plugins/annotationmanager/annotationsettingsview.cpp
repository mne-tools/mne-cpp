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
#include <QMenu>

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
, m_pStrListModel(QSharedPointer<QStringListModel>::create())
, m_pColordialog(new QColorDialog(this))
{
    m_pUi->setupUi(this);
    this->setMinimumWidth(330);
    this->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Preferred));

    m_pUi->m_comboBox_filterTypes->addItem("All");
    m_pUi->m_comboBox_filterTypes->addItem("0");
    m_pUi->m_comboBox_filterTypes->setCurrentText("All");

    m_pUi->m_comboBox_filterTypes->hide();
    m_pUi->line->hide();
    m_pUi->m_label_filterEvents->hide();

    onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::reset()
{
    setModel(QSharedPointer<ANSHAREDLIB::AnnotationModel>::create());
}

//=============================================================================================================

void AnnotationSettingsView::initMSVCSettings()
{
    //Model
    m_pUi->m_tableView_eventTableView->setModel(m_pAnnModel.data());
    connect(m_pAnnModel.data(),&ANSHAREDLIB::AnnotationModel::dataChanged,
            this, &AnnotationSettingsView::onDataChanged, Qt::UniqueConnection);

    m_pUi->m_listView_groupListView->setModel(m_pStrListModel.data());

    //Delegate
    m_pAnnDelegate = QSharedPointer<AnnotationDelegate>(new AnnotationDelegate(this));
    m_pUi->m_tableView_eventTableView->setItemDelegate(m_pAnnDelegate.data());

    connect(m_pAnnDelegate.data(), &AnnotationDelegate::sampleValueChanged,
            this, &AnnotationSettingsView::realTimeDataSample, Qt::UniqueConnection);

    connect(m_pAnnDelegate.data(), &AnnotationDelegate::timeValueChanged,
            this, &AnnotationSettingsView::realTimeDataTime, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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

    //'Show all' checkbox
    connect(m_pUi->m_checkBox_showAll, &QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onShowAllChecked, Qt::UniqueConnection);

    //Annotation types combo box
    connect(m_pUi->m_comboBox_filterTypes, &QComboBox::currentTextChanged,
            m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::setEventFilterType, Qt::UniqueConnection);
    connect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::updateEventTypes,
            this, &AnnotationSettingsView::updateComboBox, Qt::UniqueConnection);

    //Add type button
    connect(m_pUi->m_pushButton_addEventType, &QPushButton::clicked,
            this, &AnnotationSettingsView::addNewAnnotationType, Qt::UniqueConnection);

    //Save button
    connect(m_pUi->m_pushButtonSave, &QPushButton::clicked,
            this, &AnnotationSettingsView::onSaveButton, Qt::UniqueConnection);

    //Delete button
    connect(m_pUi->m_pushButtonDelete, &QPushButton::clicked,
            this, &AnnotationSettingsView::removeAnnotationfromModel, Qt::UniqueConnection);

    //Adding Events
    connect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::addNewAnnotation,
            this, &AnnotationSettingsView::addAnnotationToModel, Qt::UniqueConnection);

    //Switching groups
    connect(m_pUi->m_listView_groupListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationSettingsView::groupChanged, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customContextMenuRequested, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pUi->m_tableView_eventTableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

//=============================================================================================================

void AnnotationSettingsView::onActiveEventsChecked(int iCheckBoxState)
{
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

void AnnotationSettingsView::addAnnotationToModel()
{
    qDebug() << "AnnotationSettingsView::addAnnotationToModel -- Here";

    if (m_pStrListModel->stringList().isEmpty()) {
        newUserGroup("User Made");
    } /*else {
        if(m_pAnnModel->isUserMade()){
            //Do nothing
        } else {
            for (int i = 0; i < m_pAnnModel->getHubSize(); i++){
                if (
        }
    }*/

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
    m_pUi->m_tableView_eventTableView->update();
    m_pUi->m_tableView_eventTableView->repaint();
    m_pUi->m_tableView_eventTableView->viewport()->update();
    m_pUi->m_tableView_eventTableView->viewport()->repaint();
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::passFiffParams(int iFirst,
                                            int iLast,
                                            float fFreq)
{
    m_pAnnModel->setFirstLastSample(iFirst, iLast);
    m_pAnnModel->setSampleFreq(fFreq);
}

//=============================================================================================================

void AnnotationSettingsView::removeAnnotationfromModel()
{
    QModelIndexList indexList = m_pUi->m_tableView_eventTableView->selectionModel()->selectedIndexes();

    int iTracker = 9999;
    for(int i = indexList.size() - 1; i >= 0; i--) {
        qDebug() << "Removing Index" << indexList.at(i).row();

        if (indexList.at(i).row() == iTracker){
            continue;
        }

        m_pAnnModel->removeRow(indexList.at(i).row());
        iTracker = indexList.at(i).row();
    }

    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::addNewAnnotationType()
{
    m_pAnnModel->addNewAnnotationType(QString().number(m_pUi->m_spinBox_addEventType->value()),
                                      m_pColordialog->getColor(Qt::black, this));

    newUserGroup(m_pUi->lineEdit->text(),
                 m_pUi->m_spinBox_addEventType->value());

    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::onSelectedEventsChecked(int iCheckBoxState)
{
    m_iCheckSelectedState = iCheckBoxState;
    m_pAnnModel->setShowSelected(m_iCheckSelectedState);

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
    disconnect(m_pAnnModel.data(), &ANSHAREDLIB::AnnotationModel::addNewAnnotation,
            this, &AnnotationSettingsView::addAnnotationToModel);
    disconnect(m_pUi->m_listView_groupListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationSettingsView::groupChanged);
    disconnect(m_pUi->m_checkBox_showAll, &QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onShowAllChecked);
    disconnect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customContextMenuRequested);

}

//=============================================================================================================

void AnnotationSettingsView::onCurrentSelectedChanged()
{
    m_pAnnModel->clearSelected();
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());

    for (int i = 0;  i < m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().size(); i++) {
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
    m_pAnnModel->updateFilteredSample(iValue);
    this->onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::realTimeDataTime(double dValue)
{
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());
    dValue *= m_pAnnModel->getFreq();
    int t_iSample = static_cast<int>(dValue);
    m_pAnnModel->updateFilteredSample(t_iSample);
    this->onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::newUserGroup(const QString& sName, int iType)
{
    QStringList* list = new QStringList(m_pStrListModel->stringList());
    list->append(sName);
    m_pStrListModel->setStringList(*list);

    int iCat = m_pAnnModel->createGroup(sName, true, iType);

    m_pUi->m_listView_groupListView->selectionModel()->select(m_pStrListModel->index(iCat), QItemSelectionModel::ClearAndSelect);
}

//=============================================================================================================

void AnnotationSettingsView::groupChanged()
{
    if(!m_pUi->m_listView_groupListView->selectionModel()->selectedRows().size()){
        return;
    }

    if(m_pUi->m_checkBox_showAll->isChecked()){
        m_pUi->m_checkBox_showAll->setCheckState(Qt::Unchecked);
    }

    m_pAnnModel->switchGroup(m_pUi->m_listView_groupListView->selectionModel()->selectedRows().at(0).row());
    m_pUi->m_listView_groupListView->repaint();
    m_pUi->m_tableView_eventTableView->reset();
    this->onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::onShowAllChecked(int iCheckBoxState)
{
    if (iCheckBoxState){
        m_pUi->m_listView_groupListView->clearSelection();
        m_pAnnModel->showAll(iCheckBoxState);
        m_pUi->m_tableView_eventTableView->reset();
        this->onDataChanged();
    } else {
        m_pAnnModel->hideAll();
        this->onDataChanged();
    }
}

//=============================================================================================================

void AnnotationSettingsView::customContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);

    QAction* markTime = menu->addAction(tr("Delete Event"));
    connect(markTime, &QAction::triggered,
            this, &AnnotationSettingsView::removeAnnotationfromModel, Qt::UniqueConnection);

    menu->popup(m_pUi->m_tableView_eventTableView->viewport()->mapToGlobal(pos));
}
