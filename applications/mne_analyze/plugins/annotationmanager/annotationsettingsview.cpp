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

#include <fiff/fiff.h>
#include <rtprocessing/detecttrigger.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <disp/viewers/triggerdetectionview.h>

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
#include <QMessageBox>
#include <QInputDialog>
#include <QtConcurrent/QtConcurrent>

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
, m_pFiffRawModel(Q_NULLPTR)
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

    m_pTriggerDetectView = QSharedPointer<DISPLIB::TriggerDetectionView>(new DISPLIB::TriggerDetectionView("MNEANALYZE/EVENTS",
                                                                                                           Q_NULLPTR,
                                                                                                           Qt::Window));
    m_pTriggerDetectView->setProcessingMode(DISPLIB::AbstractView::ProcessingMode::Offline);
    m_pTriggerDetectView->setWindowFlag(Qt::WindowStaysOnTopHint);
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

    //Switching groups
    connect(m_pUi->m_listWidget_groupListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationSettingsView::groupChanged, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customEventContextMenuRequested, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pUi->m_tableView_eventTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_pUi->m_listWidget_groupListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_listWidget_groupListWidget, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customGroupContextMenuRequested, Qt::UniqueConnection);

    connect(m_pUi->m_pushButtonStim, &QPushButton::clicked,
            this, &AnnotationSettingsView::onStimButtonClicked, Qt::UniqueConnection);

    connect(m_pTriggerDetectView.data(), &DISPLIB::TriggerDetectionView::detectTriggers,
            this, &AnnotationSettingsView::onDetectTriggers, Qt::UniqueConnection);

    connect(&m_FutureWatcher, &QFutureWatcher<QMap<double,QList<int>>>::finished,
            this, &AnnotationSettingsView::createGroupsFromTriggers, Qt::UniqueConnection);
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

void AnnotationSettingsView::addAnnotationToModel(int iSamplePos)
{
    if(!(m_pAnnModel->getHubSize())){
        newUserGroup("User Events",
                     0,
                     true);
        m_pUi->m_listWidget_groupListWidget->setCurrentRow(0);
    }

    m_pAnnModel->setSamplePos(iSamplePos);

    m_pAnnModel->insertRow(0, QModelIndex());
    emit triggerRedraw();
}

//=============================================================================================================

void AnnotationSettingsView::setModel(QSharedPointer<ANSHAREDLIB::AnnotationModel> pAnnModel)
{
    m_pAnnModel = pAnnModel;

    initMSVCSettings();
    initGUIFunctionality();
    loadGroupSettings();
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
    if(newUserGroup(m_pUi->lineEdit->text(), m_pUi->m_spinBox_addEventType->value())) {
        m_pAnnModel->addNewAnnotationType(QString().number(m_pUi->m_spinBox_addEventType->value()),
                                      QColor(Qt::black));

        emit triggerRedraw();
    }
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
    disconnect(m_pUi->m_listWidget_groupListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AnnotationSettingsView::groupChanged);
    disconnect(m_pUi->m_checkBox_showAll, &QCheckBox::stateChanged,
            this, &AnnotationSettingsView::onShowAllChecked);
    disconnect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customEventContextMenuRequested);
    disconnect(m_pUi->m_listWidget_groupListWidget, &QWidget::customContextMenuRequested,
            this, &AnnotationSettingsView::customGroupContextMenuRequested);
    disconnect(m_pUi->m_listWidget_groupListWidget, &QListWidget::itemChanged,
            this, &AnnotationSettingsView::renameGroup);
    disconnect(m_pUi->m_pushButtonStim, &QPushButton::clicked,
            this, &AnnotationSettingsView::onStimButtonClicked);
    disconnect(m_pTriggerDetectView.data(), &DISPLIB::TriggerDetectionView::detectTriggers,
            this, &AnnotationSettingsView::onDetectTriggers);


    saveGroupSettings();
}

//=============================================================================================================

void AnnotationSettingsView::onCurrentSelectedChanged()
{
    m_pAnnModel->clearSelected();
    m_pAnnModel->setSelectedAnn(m_pUi->m_tableView_eventTableView->selectionModel()->currentIndex().row());

    for (int i = 0;  i < m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().size(); i++) {
        m_pAnnModel->appendSelected(m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().at(i).row());
    }

    onDataChanged();
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

    m_pAnnModel->saveToFile(fileName);
    #endif
}

//=============================================================================================================

void AnnotationSettingsView::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case  Qt::Key_Delete:
            this->removeAnnotationfromModel();
            break;
        case Qt::Key_J:
            emit jumpToSelected();
            break;
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

bool AnnotationSettingsView::newUserGroup(const QString& sName,
                                          int iType,
                                          bool bDefaultColor)
{
    if (!(m_pUi->m_listWidget_groupListWidget->findItems(sName, Qt::MatchExactly).isEmpty())){
        //Name already in use
        QMessageBox msgBox;
        msgBox.setText("Group name already in use");
        msgBox.setInformativeText("Please select a new name");
        msgBox.exec();
        return false;
    }

    QColor groupColor;

    if (!bDefaultColor) {
        groupColor = m_pColordialog->getColor(Qt::black, this);
        if(!groupColor.isValid()){
            return false;
        }
    } else {
        groupColor = QColor(Qt::blue);
    }

    int iCat = m_pAnnModel->createGroup(sName,
                                        true,
                                        iType,
                                        groupColor);

    QListWidgetItem* newItem = new QListWidgetItem(sName);
    newItem->setData(Qt::UserRole, QVariant(iCat));
    newItem->setData(Qt::DecorationRole, groupColor);
    newItem->setFlags (newItem->flags () | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    m_pUi->m_listWidget_groupListWidget->addItem(newItem);
    emit m_pUi->m_listWidget_groupListWidget->setCurrentItem(newItem);

    m_pUi->lineEdit->setText("New Group " + QString::number(iCat + 1));

    emit groupsUpdated();
    return true;
}

//=============================================================================================================

void AnnotationSettingsView::groupChanged()
{
    if(!m_pUi->m_listWidget_groupListWidget->selectionModel()->selectedRows().size()){
        return;
    }

    if(m_pUi->m_checkBox_showAll->isChecked()){
        m_pUi->m_checkBox_showAll->setCheckState(Qt::Unchecked);
    }

    m_pAnnModel->switchGroup(m_pUi->m_listWidget_groupListWidget->selectedItems().first()->data(Qt::UserRole).toInt());
    m_pUi->m_listWidget_groupListWidget->repaint();
    m_pUi->m_tableView_eventTableView->reset();
    this->onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::onShowAllChecked(int iCheckBoxState)
{
    if (iCheckBoxState){
        m_pUi->m_listWidget_groupListWidget->clearSelection();
        m_pAnnModel->showAll(iCheckBoxState);
        m_pUi->m_tableView_eventTableView->reset();
        this->onDataChanged();
    } else {
        m_pAnnModel->hideAll();
        if(m_pUi->m_listWidget_groupListWidget->selectedItems().isEmpty()){
            m_pUi->m_listWidget_groupListWidget->setCurrentRow(0);
        }
        this->onDataChanged();
    }
}

//=============================================================================================================

void AnnotationSettingsView::customEventContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);

    QAction* deleteEvent = menu->addAction(tr("Delete event"));
    connect(deleteEvent, &QAction::triggered,
            this, &AnnotationSettingsView::removeAnnotationfromModel, Qt::UniqueConnection);

    QAction* jumpToEvent = menu->addAction(tr("Jump to event"));
    connect(jumpToEvent, &QAction::triggered,
            this, &AnnotationSettingsView::jumpToSelected, Qt::UniqueConnection);

    menu->popup(m_pUi->m_tableView_eventTableView->viewport()->mapToGlobal(pos));
}

//=============================================================================================================

void AnnotationSettingsView::customGroupContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);

    QAction* renameGroup =  menu->addAction(tr("Rename"));
    connect(renameGroup, &QAction::triggered,
            this, &AnnotationSettingsView::renameGroup, Qt::UniqueConnection);

    QAction* colorChange = menu->addAction(tr("Change color"));
    connect(colorChange, &QAction::triggered,
            this, &AnnotationSettingsView::changeGroupColor, Qt::UniqueConnection);

    QAction* markTime = menu->addAction(tr("Delete group"));
    connect(markTime, &QAction::triggered,
            this, &AnnotationSettingsView::deleteGroup, Qt::UniqueConnection);

    menu->popup(m_pUi->m_listWidget_groupListWidget->viewport()->mapToGlobal(pos));
}

//=============================================================================================================

void AnnotationSettingsView::deleteGroup()
{
    int iSelected = m_pUi->m_listWidget_groupListWidget->selectionModel()->selectedRows().first().row();
    QListWidgetItem* itemToDelete = m_pUi->m_listWidget_groupListWidget->takeItem(iSelected);
    m_pAnnModel->removeGroup(itemToDelete->data(Qt::UserRole).toInt());
    m_pUi->m_listWidget_groupListWidget->selectionModel()->clearSelection();
    delete itemToDelete;
    onDataChanged();
    emit groupsUpdated();
}

//=============================================================================================================

void AnnotationSettingsView::saveGroupSettings()
{
    while(m_pUi->m_listWidget_groupListWidget->count()){
        m_pAnnModel->pushGroup(m_pUi->m_listWidget_groupListWidget->takeItem(m_pUi->m_listWidget_groupListWidget->count() - 1));
    }
}

//=============================================================================================================

void AnnotationSettingsView::loadGroupSettings()
{
    while(m_pAnnModel->getGroupStackSize()){
        m_pUi->m_listWidget_groupListWidget->addItem(m_pAnnModel->popGroup());
    }
}

//=============================================================================================================

void AnnotationSettingsView::renameGroup()
{
    QString text = QInputDialog::getText(this, tr("Event Viewer Group Settings"),
                                             tr("Group Name:"), QLineEdit::Normal);

    if (m_pUi->m_listWidget_groupListWidget->findItems(text, Qt::MatchExactly).size() > 0){
        QMessageBox* msgBox = new QMessageBox();
        msgBox->setText("Group name already in use");
        msgBox->setInformativeText("Please select a new name");
        msgBox->open();
    }else if(text.isEmpty()){
        QMessageBox* msgBox = new QMessageBox();
        msgBox->setText("Group name not valid");
        msgBox->setInformativeText("Please select a new name");
        msgBox->open();
    }else {
        m_pUi->m_listWidget_groupListWidget->currentItem()->setText(text);
        m_pAnnModel->setGroupName(m_pUi->m_listWidget_groupListWidget->currentItem()->data(Qt::UserRole).toInt(), text);
        emit groupsUpdated();
    }
}

//=============================================================================================================

void AnnotationSettingsView::changeGroupColor()
{
    QColor groupColor = m_pColordialog->getColor(Qt::black, this);

    if(!groupColor.isValid()){
        return;
    }

    int iSelected = m_pUi->m_listWidget_groupListWidget->selectionModel()->selectedRows().first().row();
    QListWidgetItem* itemToChange = m_pUi->m_listWidget_groupListWidget->item(iSelected);

    itemToChange->setData(Qt::DecorationRole, groupColor);
    m_pAnnModel->setGroupColor(itemToChange->data(Qt::UserRole).toInt(),
                               groupColor);
    onDataChanged();
}

//=============================================================================================================

void AnnotationSettingsView::onStimButtonClicked()
{
    if(m_pTriggerDetectView->isHidden()){
        m_pTriggerDetectView->activateWindow();
        m_pTriggerDetectView->show();
        m_pTriggerDetectView->resize(m_pTriggerDetectView->minimumSizeHint());
    }
}

//=============================================================================================================

void AnnotationSettingsView::initTriggerDetect(const QSharedPointer<FIFFLIB::FiffInfo>info)
{
    m_pTriggerDetectView->init(info);
}

//=============================================================================================================

void AnnotationSettingsView::onDetectTriggers(const QString &sChannelName,
                                              double dThreshold)
{
    if(m_FutureWatcher.isRunning()){
        return;
    }

    emit loadingStart("Detecting triggers...");

    m_Future = QtConcurrent::run(this,
                                 &AnnotationSettingsView::detectTriggerCalculations,
                                 sChannelName,
                                 dThreshold,
                                 *m_pFiffRawModel->getFiffInfo(),
                                 *this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data());
    m_FutureWatcher.setFuture(m_Future);

}

//=============================================================================================================

QMap<double,QList<int>> AnnotationSettingsView::detectTriggerCalculations(const QString& sChannelName,
                                                                          double dThreshold,
                                                                          FIFFLIB::FiffInfo fiffInfo,
                                                                          FIFFLIB::FiffRawData fiffRaw)
{
    int iCurrentTriggerChIndex = 9999;

    for(int i = 0; i < fiffInfo.chs.size(); ++i) {
        if(fiffInfo.chs[i].ch_name == sChannelName) {
            iCurrentTriggerChIndex = i;
            break;
        }
    }

    if(iCurrentTriggerChIndex == 9999){
        qWarning() << "[AnnotationSettingsView::onDetectTriggers] Channel Index not valid";\
        QMap<double,QList<int>> map;
        return map;
    }

    Eigen::MatrixXd mSampleData, mSampleTimes;

    fiffRaw.read_raw_segment(mSampleData,
                               mSampleTimes);

    QList<QPair<int,double>> detectedTriggerSamples = RTPROCESSINGLIB::detectTriggerFlanksMax(mSampleData,
                                                                                              iCurrentTriggerChIndex,
                                                                                              0,
                                                                                              dThreshold,
                                                                                              0);

    QMap<double,QList<int>> mEventsinTypes;

    for(QPair<int,double> pair : detectedTriggerSamples){
        mEventsinTypes[pair.second].append(pair.first);
    }

    return mEventsinTypes;
}

//=============================================================================================================


void AnnotationSettingsView::onNewFiffRawViewModel(QSharedPointer<ANSHAREDLIB::FiffRawViewModel> pFiffRawModel)
{
    m_pFiffRawModel = pFiffRawModel;

    passFiffParams(pFiffRawModel->absoluteFirstSample(),
                   pFiffRawModel->absoluteLastSample(),
                   pFiffRawModel->getFiffInfo()->sfreq);

    initTriggerDetect(m_pFiffRawModel->getFiffInfo());
}

//=============================================================================================================

bool AnnotationSettingsView::newStimGroup(const QString &sName,
                                          int iType,
                                          const QColor &groupColor)
{
    int iCat = m_pAnnModel->createGroup(sName + "_" + QString::number(iType),
                                        false,
                                        iType,
                                        groupColor);

    QListWidgetItem* newItem = new QListWidgetItem(sName + "_" + QString::number(iType));
    newItem->setData(Qt::UserRole, QVariant(iCat));
    newItem->setData(Qt::DecorationRole, groupColor);
    newItem->setFlags (newItem->flags () | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    m_pUi->m_listWidget_groupListWidget->addItem(newItem);
    emit m_pUi->m_listWidget_groupListWidget->setCurrentItem(newItem);

    return true;
}

//=============================================================================================================

void AnnotationSettingsView::createGroupsFromTriggers()
{
    QMap<double,QList<int>> mEventGroupMap = m_Future.result();

    QList<double> keyList = mEventGroupMap.keys();
    int iFirstSample = m_pFiffRawModel->absoluteFirstSample();

    QColor colors[10] = {QColor("cyan"), QColor("magenta"), QColor("red"),
                          QColor("darkRed"), QColor("darkCyan"), QColor("darkMagenta"),
                          QColor("green"), QColor("darkGreen"), QColor("yellow"),
                          QColor("blue")};

    for (int i = 0; i < keyList.size(); i++){
        if ((m_pUi->m_listWidget_groupListWidget->findItems(m_pTriggerDetectView->getSelectedStimChannel()+ "_" + QString::number(static_cast<int>(keyList[i])), Qt::MatchExactly).isEmpty())){
            newStimGroup(m_pTriggerDetectView->getSelectedStimChannel(), static_cast<int>(keyList[i]), colors[i % 10]);
            groupChanged();
            for (int j : mEventGroupMap[keyList[i]]){
                m_pAnnModel->setSamplePos(j + iFirstSample);
                m_pAnnModel->insertRow(0, QModelIndex());
            }
        }
    }

    emit triggerRedraw();
    emit groupsUpdated();
    emit loadingEnd("Detecting triggers...");
}
