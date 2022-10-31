//=============================================================================================================
/**
 * @file     eventview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.9
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Christoph Dinh, Lorenz Esch, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Definition of the EventView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventview.h"
#include "ui_eventview.h"

#include <fiff/fiff.h>
#include <rtprocessing/detecttrigger.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <disp/viewers/triggerdetectionview.h>

#include <set>

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

EventView::EventView()
: m_pUi(new Ui::EventWindowDockWidget)
, m_iCheckState(0)
, m_iLastSampClicked(0)
, m_pEventModel(Q_NULLPTR)
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
    m_pUi->m_spinBox_addEventType->hide();
    m_pUi->label_2->hide();

    onDataChanged();
    createContextMenu();

    m_pTriggerDetectView = QSharedPointer<DISPLIB::TriggerDetectionView>(new DISPLIB::TriggerDetectionView("MNEANALYZE/EVENTS",
                                                                                                           Q_NULLPTR,
                                                                                                           Qt::Window));
    m_pTriggerDetectView->setProcessingMode(DISPLIB::AbstractView::ProcessingMode::Offline);
    m_pTriggerDetectView->setWindowFlag(Qt::WindowStaysOnTopHint);
}

//=============================================================================================================

EventView::~EventView()
{
    delete m_pEventContexMenu;
    delete m_pGroupContexMenu;
}

//=============================================================================================================

void EventView::reset()
{
    setModel(QSharedPointer<ANSHAREDLIB::EventModel>::create());
}

//=============================================================================================================

void EventView::initMVCSettings()
{
    //Model
    m_pUi->m_tableView_eventTableView->setModel(m_pEventModel.data());
    connect(m_pEventModel.data(),&ANSHAREDLIB::EventModel::dataChanged,
            this, &EventView::onDataChanged, Qt::UniqueConnection);

    //Delegate
    m_pAnnDelegate = QSharedPointer<EventDelegate>(new EventDelegate(this));
    m_pUi->m_tableView_eventTableView->setItemDelegate(m_pAnnDelegate.data());

    m_pUi->m_tableView_eventTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pUi->m_tableView_eventTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(m_pEventModel.data(), &ANSHAREDLIB::EventModel::eventGroupsUpdated,
            this, &EventView::redrawGroups, Qt::UniqueConnection);
}

//=============================================================================================================

void EventView::initGUIFunctionality()
{
    //'Activate events' checkbox
    connect(m_pUi->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &EventView::activeEventsChecked, Qt::UniqueConnection);

    //'Show selected event' checkbox
    connect(m_pUi->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &EventView::onSelectedEventsChecked, Qt::UniqueConnection);
    connect(m_pUi->m_tableView_eventTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EventView::onCurrentSelectedChanged, Qt::UniqueConnection);

    //'Show all' checkbox
//    connect(m_pUi->m_checkBox_showAll, &QCheckBox::stateChanged,
//            this, &EventView::onShowAllChecked, Qt::UniqueConnection);

    //Add type button
    connect(m_pUi->m_pushButton_addEventType, &QPushButton::clicked,
            this, &EventView::addEventGroup, Qt::UniqueConnection);

    //Switching groups
    connect(m_pUi->m_listWidget_groupListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EventView::groupChanged, Qt::UniqueConnection);

    //Init custom context menus
    m_pUi->m_tableView_eventTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &EventView::customEventContextMenuRequested, Qt::UniqueConnection);

    m_pUi->m_tableView_eventTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pUi->m_tableView_eventTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_pUi->m_listWidget_groupListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_listWidget_groupListWidget, &QWidget::customContextMenuRequested,
            this, &EventView::customGroupContextMenuRequested, Qt::UniqueConnection);

//    connect(m_pUi->m_listWidget_groupListWidget, &QListWidget::currentTextChanged,
//            this, &EventView::renameGroup, Qt::UniqueConnection);

    connect(m_pUi->m_listWidget_groupListWidget, &QListWidget::itemChanged,
                this, &EventView::onGroupItemNameChanged, Qt::UniqueConnection);

    connect(m_pUi->m_pushButtonStim, &QPushButton::clicked,
            this, &EventView::onStimButtonClicked, Qt::UniqueConnection);

    connect(m_pTriggerDetectView.data(), &DISPLIB::TriggerDetectionView::detectTriggers,
            this, &EventView::onDetectTriggers, Qt::UniqueConnection);

    connect(&m_FutureWatcher, &QFutureWatcher<QMap<double,QList<int>>>::finished,
            this, &EventView::createGroupsFromTriggers, Qt::UniqueConnection);

    m_pEventModel->setShowSelected(m_pUi->m_checkBox_showSelectedEventsOnly->isChecked());
}

//=============================================================================================================

void EventView::onActiveEventsChecked(int iCheckBoxState)
{
    m_iCheckState = iCheckBoxState;
    emit activeEventsChecked(m_iCheckState);
}

//=============================================================================================================

void EventView::addEventToModel(int iSamplePos)
{
    if(!m_pEventModel){
        return;
    }

    if(!m_pEventModel->getNumberOfGroups()){
        addEventGroup();
    }

    m_pEventModel->addEvent(iSamplePos);

    emit triggerRedraw();
}

//=============================================================================================================

void EventView::setModel(QSharedPointer<ANSHAREDLIB::EventModel> pEventModel)
{
    m_pEventModel = pEventModel;

    initMVCSettings();
    initGUIFunctionality();
    redrawGroups();
//    loadGroupSettings();

    if(m_pFiffRawModel){
        m_pEventModel->setSharedMemory(m_pFiffRawModel->isRealtime());
    }

    onDataChanged();
}

//=============================================================================================================

void EventView::onDataChanged()
{
    m_pUi->m_tableView_eventTableView->update();
    m_pUi->m_tableView_eventTableView->repaint();
    m_pUi->m_tableView_eventTableView->viewport()->update();
    m_pUi->m_tableView_eventTableView->viewport()->repaint();
    emit triggerRedraw();
    emit eventsUpdated();
}

//=============================================================================================================

void EventView::passFiffParams(int iFirst,
                               int iLast,
                               float fFreq)
{
    m_pEventModel->setFirstLastSample(iFirst, iLast);
    m_pEventModel->setSampleFreq(fFreq);
}

//=============================================================================================================

void EventView::removeEvent()
{
    QModelIndexList indexList = m_pUi->m_tableView_eventTableView->selectionModel()->selectedIndexes();

    std::set<int> set;

    for (auto index : indexList){
        set.insert(index.row());
    }

    for (auto it = set.crbegin(); it != set.crend(); ++it){
        m_pEventModel->removeRow(*it);
    }

    emit triggerRedraw();
}

//=============================================================================================================

void EventView::addEventGroup()
{
    newUserGroup(m_pUi->lineEdit->text(), m_pUi->m_spinBox_addEventType->value());
}

//=============================================================================================================

void EventView::onSelectedEventsChecked(int iCheckBoxState)
{
    m_iCheckSelectedState = iCheckBoxState;
    m_pEventModel->setShowSelected(m_iCheckSelectedState);

    emit triggerRedraw();
}

//=============================================================================================================

void EventView::disconnectFromModel()
{
    disconnect(m_pEventModel.data(),&ANSHAREDLIB::EventModel::dataChanged,
            this, &EventView::onDataChanged);
    disconnect(m_pUi->m_checkBox_activateEvents, &QCheckBox::stateChanged,
            this, &EventView::activeEventsChecked);
    disconnect(m_pUi->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged,
            this, &EventView::onSelectedEventsChecked);
    disconnect(m_pUi->m_pushButton_addEventType, &QPushButton::clicked,
            this, &EventView::addEventGroup);
    disconnect(m_pUi->m_listWidget_groupListWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EventView::groupChanged);
//    disconnect(m_pUi->m_checkBox_showAll, &QCheckBox::stateChanged,
//            this, &EventView::onShowAllChecked);
    disconnect(m_pUi->m_tableView_eventTableView, &QWidget::customContextMenuRequested,
            this, &EventView::customEventContextMenuRequested);
    disconnect(m_pUi->m_listWidget_groupListWidget, &QWidget::customContextMenuRequested,
            this, &EventView::customGroupContextMenuRequested);
    disconnect(m_pUi->m_pushButtonStim, &QPushButton::clicked,
            this, &EventView::onStimButtonClicked);
    disconnect(m_pTriggerDetectView.data(), &DISPLIB::TriggerDetectionView::detectTriggers,
            this, &EventView::onDetectTriggers);
    disconnect(m_pEventModel.data(), &ANSHAREDLIB::EventModel::eventGroupsUpdated,
            this, &EventView::redrawGroups);
}

//=============================================================================================================

void EventView::onCurrentSelectedChanged()
{
    m_pEventModel->clearEventSelection();

    for (int i = 0;  i < m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().size(); i++) {
        m_pEventModel->appendSelected(m_pUi->m_tableView_eventTableView->selectionModel()->selectedRows().at(i).row());
    }

    onDataChanged();
}

//=============================================================================================================

void EventView::onSaveButton()
{
    #ifdef WASMBUILD
    m_pEventModel->saveToFile("");
    #else
    QString fileName = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                    tr("Save Events"), "",
                                                    tr("Event file (*.eve);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    m_pEventModel->saveToFile(fileName);
    #endif
}

//=============================================================================================================

void EventView::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case  Qt::Key_Delete:
            if(m_pUi->m_tableView_eventTableView->hasFocus()){
                removeEvent();
            } else if(m_pUi->m_listWidget_groupListWidget->hasFocus()){
                deleteGroup();
            }
            break;
        case Qt::Key_J:
            emit jumpToSelected();
            break;
    }
}

//=============================================================================================================

bool EventView::newUserGroup(const QString& sName,
                                          int iType,
                                          bool bDefaultColor)
{
    if(!m_pEventModel){
        return false;
    }

    QColor groupColor;

    if (!bDefaultColor) {
        groupColor = QColor(rand()%255, rand()%255, rand()%255);
        if(!groupColor.isValid()){
            return false;
        }
    } else {
        groupColor = QColor(Qt::blue);
    }

    m_pEventModel->addGroup(sName, groupColor);

    m_pUi->lineEdit->setText("New Group"/* + QString::number(iCat + 1)*/);

    return true;
}

//=============================================================================================================

void EventView::groupChanged()
{    
    auto selection = m_pUi->m_listWidget_groupListWidget->selectionModel()->selectedRows();

    if(!selection.size()){
        return;
    }

    m_pEventModel->updateSelectedGroups(selection);
}

//=============================================================================================================

void EventView::customEventContextMenuRequested(const QPoint &pos)
{
    if(m_pEventContexMenu){
        m_pEventContexMenu->popup(m_pUi->m_tableView_eventTableView->viewport()->mapToGlobal(pos));
    }
}

//=============================================================================================================

void EventView::customGroupContextMenuRequested(const QPoint &pos)
{
    if(m_pGroupContexMenu){
        m_pGroupContexMenu->popup(m_pUi->m_listWidget_groupListWidget->viewport()->mapToGlobal(pos));
    }
}

//=============================================================================================================

void EventView::deleteGroup()
{
    m_pEventModel->deleteSelectedGroups();
}

//=============================================================================================================

void EventView::renameGroup(const QString &currentText)
{
    if(m_pEventModel){
        m_pEventModel->setSelectedGroupName(currentText);
    }
}

//=============================================================================================================

void EventView::changeGroupColor()
{
    QColor groupColor = m_pColordialog->getColor(Qt::black, this);

    if(!groupColor.isValid()){
        return;
    }

    m_pEventModel->setGroupColor(groupColor);
}

//=============================================================================================================

void EventView::onStimButtonClicked()
{
    if(m_pTriggerDetectView->isHidden()){
        m_pTriggerDetectView->activateWindow();
        m_pTriggerDetectView->show();
        m_pTriggerDetectView->resize(m_pTriggerDetectView->minimumSizeHint());
    }
}

//=============================================================================================================

void EventView::initTriggerDetect(const QSharedPointer<FIFFLIB::FiffInfo>info)
{
    m_pTriggerDetectView->init(info);
}

//=============================================================================================================

void EventView::onDetectTriggers(const QString &sChannelName,
                                              double dThreshold)
{
    if (!m_pFiffRawModel) {
        qWarning() << "[EventView::onDetectTriggers] No Fiff Raw Model selected for trigger detection.";
        return;
    }

    if(m_FutureWatcher.isRunning()){
        return;
    }

    emit loadingStart("Detecting triggers...");

    m_Future = QtConcurrent::run(this,
                                 &EventView::detectTriggerCalculations,
                                 sChannelName,
                                 dThreshold,
                                 *m_pFiffRawModel->getFiffInfo(),
                                 *this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data());
    m_FutureWatcher.setFuture(m_Future);

}

//=============================================================================================================

QMap<double,QList<int>> EventView::detectTriggerCalculations(const QString& sChannelName,
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
        qWarning() << "[EventView::onDetectTriggers] Channel Index not valid";\
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


void EventView::onNewFiffRawViewModel(QSharedPointer<ANSHAREDLIB::FiffRawViewModel> pFiffRawModel)
{
    m_pFiffRawModel = pFiffRawModel;

    if(auto info = pFiffRawModel->getFiffInfo()){
    passFiffParams(pFiffRawModel->absoluteFirstSample(),
                   pFiffRawModel->absoluteLastSample(),
                   pFiffRawModel->getFiffInfo()->sfreq);
    }

    initTriggerDetect(m_pFiffRawModel->getFiffInfo());
}

//=============================================================================================================

bool EventView::newStimGroup(const QString &sName,
                                          int iType,
                                          const QColor &groupColor)
{
    m_pEventModel->addGroup(sName + "_" + QString::number(iType), groupColor);

    return true;
}

//=============================================================================================================

void EventView::createGroupsFromTriggers()
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
            newStimGroup(m_pTriggerDetectView->getSelectedStimChannel(),
                         static_cast<int>(keyList[i]),
                         colors[i % 10]);
            for (int j : mEventGroupMap[keyList[i]]){
                m_pEventModel->addEvent(j + iFirstSample);
            }
        }
    }

    emit triggerRedraw();
    emit loadingEnd("Detecting triggers...");
}

//=============================================================================================================

void EventView::clearView(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if (qSharedPointerCast<ANSHAREDLIB::AbstractModel>(m_pEventModel) == pRemovedModel){
        disconnectFromModel();
        reset();
    } else if (qSharedPointerCast<ANSHAREDLIB::AbstractModel>(m_pFiffRawModel) == pRemovedModel){
        m_pFiffRawModel = Q_NULLPTR;
    }
}

//=============================================================================================================

void EventView::redrawGroups()
{
    if(!m_pEventModel){
        return;
    }
    auto groups = m_pEventModel->getGroupsToDisplay();
    auto selection = m_pEventModel->getSelectedGroups();

    m_pUi->m_listWidget_groupListWidget->clear();

    for (auto& eventGroup : *groups){
        QListWidgetItem* newItem = new QListWidgetItem(QString::fromStdString(eventGroup.name));
        newItem->setData(Qt::UserRole, QVariant(eventGroup.id));
        newItem->setData(Qt::DecorationRole, QColor(eventGroup.color.r, eventGroup.color.g, eventGroup.color.b));
        newItem->setFlags (newItem->flags () | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

        m_pUi->m_listWidget_groupListWidget->addItem(newItem);
        for (auto group : selection){
            if (group == eventGroup.id){
                m_pUi->m_listWidget_groupListWidget->setCurrentItem(newItem);
            }
        }
    }

    qDebug() << "EventView::redrawGroups";
    emit triggerRedraw();
}

//=============================================================================================================

void EventView::onGroupItemNameChanged(QListWidgetItem *item)
{
    int iGroupId = item->data(Qt::UserRole).toInt();

    if(m_pEventModel){
        m_pEventModel->setGroupName(iGroupId, item->text());
    }
}

//=============================================================================================================

void EventView::createContextMenu()
{
    //Event view custom right click menu
    m_pEventContexMenu = new QMenu(this);

    QAction* deleteEvent = m_pEventContexMenu->addAction(tr("Delete event"));
    connect(deleteEvent, &QAction::triggered,
            this, &EventView::removeEvent, Qt::UniqueConnection);

    QAction* jumpToEvent = m_pEventContexMenu->addAction(tr("Jump to event"));
    connect(jumpToEvent, &QAction::triggered,
            this, &EventView::jumpToSelected, Qt::UniqueConnection);

    //Group view custom right click menu
    m_pGroupContexMenu = new QMenu(this);

    QAction* colorChange = m_pGroupContexMenu->addAction(tr("Change color"));
    connect(colorChange, &QAction::triggered,
            this, &EventView::changeGroupColor, Qt::UniqueConnection);

    QAction* markTime = m_pGroupContexMenu->addAction(tr("Delete group"));
    connect(markTime, &QAction::triggered,
            this, &EventView::deleteGroup, Qt::UniqueConnection);
}
