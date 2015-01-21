//=============================================================================================================
/**
* @file     selectionmanagerwindow.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2014
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
* @brief    Contains the implementation of the SelectionManagerWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionmanagerwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionManagerWindow::SelectionManagerWindow(QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::SelectionManagerWindow)
{
    ui->setupUi(this);

    //Init gui elements
    initListWidgets();
    initSelectionSceneView();
    initComboBoxes();
}


//*************************************************************************************************************

SelectionManagerWindow::~SelectionManagerWindow()
{
    delete ui;
}


//*************************************************************************************************************

void SelectionManagerWindow::setCurrentlyMappedFiffChannels(const QStringList &mappedLayoutChNames)
{
    m_currentlyLoadedFiffChannels = mappedLayoutChNames;

    //Clear the visible channel list
    ui->m_listWidget_visibleChannels->clear();

    //Keep the entry All in the selection list and m_selectionGroupsMap -> delete the rest
    ui->m_listWidget_selectionGroups->clear();

    //Create group 'All' manually (bcause this group depends on the loaded channels from the fiff data file, not on the loaded selection file)
    m_selectionGroupsMap["All"] = m_currentlyLoadedFiffChannels;

    //Add selection groups to list widget
    QMapIterator<QString, QStringList> selectionIndex(m_selectionGroupsMap);
    while (selectionIndex.hasNext()) {
        selectionIndex.next();
        ui->m_listWidget_selectionGroups->insertItem(ui->m_listWidget_selectionGroups->count(), selectionIndex.key());
    }

    //Set group all as slected item
    ui->m_listWidget_selectionGroups->setCurrentItem(getItemForChName(ui->m_listWidget_selectionGroups, "All"), QItemSelectionModel::Select);

    //Update selection
    updateSelectionGroupsList(getItemForChName(ui->m_listWidget_selectionGroups, "All"), new QListWidgetItem());
}


//*************************************************************************************************************

void SelectionManagerWindow::highlightChannels(QStringList channelList)
{
    QList<QGraphicsItem *> allSceneItems = m_pSelectionScene->items();

    for(int i = 0; i<allSceneItems.size(); i++) {
        SelectionSceneItem* item = static_cast<SelectionSceneItem*>(allSceneItems.at(i));
        if(channelList.contains(item->m_sChannelName))
            item->m_bHighlightItem = true;
        else
            item->m_bHighlightItem = false;
    }

    m_pSelectionScene->update();
}


//*************************************************************************************************************

void SelectionManagerWindow::selectChannels(QStringList channelList)
{
    QList<QGraphicsItem *> allSceneItems = m_pSelectionScene->items();

    for(int i = 0; i<allSceneItems.size(); i++) {
        SelectionSceneItem* item = static_cast<SelectionSceneItem*>(allSceneItems.at(i));
        if(channelList.contains(item->m_sChannelName))
            item->setSelected(true);
        else
            item->setSelected(false);
    }

    m_pSelectionScene->update();
}


//*************************************************************************************************************

QStringList SelectionManagerWindow::getSelectedChannels()
{
    //if no channels have been selected by the user -> show selected group channels
    QListWidget* targetListWidget;
    if(ui->m_listWidget_userDefined->count()>0)
        targetListWidget = ui->m_listWidget_userDefined;
    else
        targetListWidget = ui->m_listWidget_visibleChannels;

    //Create list of channels which are to be visible in the view
    QStringList selectedChannels;

    for(int i = 0; i<targetListWidget->count(); i++) {
        QListWidgetItem* item = targetListWidget->item(i);
        selectedChannels << item->text();
    }

    return selectedChannels;
}


//*************************************************************************************************************

QListWidgetItem* SelectionManagerWindow::getItemForChName(QListWidget* listWidget, QString channelName)
{
    for(int i=0; i<listWidget->count(); i++)
        if(listWidget->item(i)->text() == channelName)
            return listWidget->item(i);

    return new QListWidgetItem();
}


//*************************************************************************************************************

const QMap<QString,QPointF>& SelectionManagerWindow::getLayoutMap()
{
    return m_layoutMap;
}


//*************************************************************************************************************

void SelectionManagerWindow::newFiffFileLoaded()
{
    loadLayout(ui->m_comboBox_layoutFile->currentText());
}


//*************************************************************************************************************

void SelectionManagerWindow::initListWidgets()
{
    //Install event filter to receive key press events
    ui->m_listWidget_userDefined->installEventFilter(this);

    //Connect list widgets to update themselves and other list widgets when changed
    connect(ui->m_listWidget_selectionGroups, &QListWidget::currentItemChanged,
                this, &SelectionManagerWindow::updateSelectionGroupsList);

    //Update data view whenever a drag and drop item movement is performed - TODO: This is inefficient because updateDataView is called everytime the list's viewport is entered
    connect(ui->m_listWidget_userDefined->model(), &QAbstractTableModel::dataChanged,
                this, &SelectionManagerWindow::updateDataView);
}


//*************************************************************************************************************

void SelectionManagerWindow::initSelectionSceneView()
{    
    //Create layout scene and set to view
    m_pSelectionScene = new SelectionScene(ui->m_graphicsView_layoutPlot);
    ui->m_graphicsView_layoutPlot->setScene(m_pSelectionScene);

    connect(m_pSelectionScene, &QGraphicsScene::selectionChanged,
                this, &SelectionManagerWindow::updateUserDefinedChannelsList);
}


//*************************************************************************************************************

void SelectionManagerWindow::initComboBoxes()
{
    ui->m_comboBox_layoutFile->clear();
    ui->m_comboBox_layoutFile->insertItems(0, QStringList()
     << QApplication::translate("SelectionManagerWindow", "babymeg_all.lout", 0)
     << QApplication::translate("SelectionManagerWindow", "Vectorview-grad.lout", 0)
     << QApplication::translate("SelectionManagerWindow", "Vectorview-all.lout", 0)
     << QApplication::translate("SelectionManagerWindow", "Vectorview-mag.lout", 0)
//     << QApplication::translate("SelectionManagerWindow", "CTF-275.lout", 0)
//     << QApplication::translate("SelectionManagerWindow", "magnesWH3600.lout", 0)
    );
    ui->m_comboBox_selectionFiles->clear();
    ui->m_comboBox_selectionFiles->insertItems(0, QStringList()
     << QApplication::translate("SelectionManagerWindow", "mne_browse_raw_vv.sel", 0)
     << QApplication::translate("SelectionManagerWindow", "mne_browse_raw_vv_new.sel", 0)
//     << QApplication::translate("SelectionManagerWindow", "mne_browse_raw_CTF_275.sel", 0)
//     << QApplication::translate("SelectionManagerWindow", "mne_browse_raw_Magnes_3600WH.sel", 0)
    );

    //Connect the layout and selection group loader
    connect(ui->m_comboBox_selectionFiles, &QComboBox::currentTextChanged,
                this, &SelectionManagerWindow::loadSelectionGroups);

    connect(ui->m_comboBox_layoutFile, &QComboBox::currentTextChanged,
                this, &SelectionManagerWindow::loadLayout);

    //Initialise layout as neuromag vectorview with all channels
    loadLayout("babymeg_all.lout");
}


//*************************************************************************************************************

bool SelectionManagerWindow::loadLayout(QString path)
{
    //Read layout
    QString newPath = QCoreApplication::applicationDirPath() + path.prepend("/MNE_Browse_Raw_Resources/Templates/Layouts/");

    bool state = LayoutLoader::readMNELoutFile(newPath, m_layoutMap);

    //Load selection groups again because they need to be reinitialised every time a new layout hase been loaded
    loadSelectionGroups(ui->m_comboBox_selectionFiles->currentText());

    //if no layout for EEG is specified generate from digitizer points
    QList<QVector<double> > inputPoints;
    QList<QVector<double> > outputPoints;
    QStringList names;
    QFile out;//(/*"./MNE_Browse_Raw_Resources/Templates/ChannelSelection/*/"manualLayout.lout");

//    for(int i = 0; i<m_pChInfoModel->rowCount(); i++) {
//        QModelIndex digIndex = m_pChInfoModel->index(i,1);
//        QString chName = m_pChInfoModel->data(digIndex,ChInfoModelRoles::GetOrigChName).toString();

//        digIndex = m_pChInfoModel->index(i,8);
//        QVector3D channelDig = m_pChInfoModel->data(digIndex,ChInfoModelRoles::GetChDigitizer).value<QVector3D>();

//        digIndex = m_pChInfoModel->index(i,4);
//        int kind = m_pChInfoModel->data(digIndex,ChInfoModelRoles::GetChKind).toInt();

//        if(kind == FIFFV_EEG_CH) { //FIFFV_MEG_CH
//            QVector<double> temp;
//            temp.append(channelDig.x());
//            temp.append(channelDig.y());
//            temp.append(-channelDig.z());
//            inputPoints.append(temp);

//            names<<chName;
//        }
//    }

    float prad = 60.0;
    float width = 5.0;
    float height = 4.0;

    if(inputPoints.size()>0)
        LayoutMaker::makeLayout(inputPoints,
                                outputPoints,
                                names,
                                out,
                                true,
                                prad,
                                width,
                                height,
                                true);

    //Add new EEG points to Layout Map
    for(int i = 0; i<outputPoints.size(); i++)
        m_layoutMap[names.at(i)] = QPointF(outputPoints.at(i)[0],outputPoints.at(i)[1]);

    //Update scene
    m_pSelectionScene->repaintItems(m_layoutMap);
    m_pSelectionScene->update();
    updateSceneItems();

    //Fit to view
    ui->m_graphicsView_layoutPlot->fitInView(m_pSelectionScene->itemsBoundingRect(), Qt::KeepAspectRatio);

    if(state)
        emit loadedLayoutMap(m_layoutMap);

    return state;
}


//*************************************************************************************************************

bool SelectionManagerWindow::loadSelectionGroups(QString path)
{
    //Clear the visible channel list
    ui->m_listWidget_visibleChannels->clear();

    //Keep the entry All in the selection list and m_selectionGroupsMap -> delete the rest
    ui->m_listWidget_selectionGroups->clear();

    //Read selection from file and store to map
    SelectionLoader* manager = new SelectionLoader();
    QString newPath = QCoreApplication::applicationDirPath() + path.prepend("/MNE_Browse_Raw_Resources/Templates/ChannelSelection/");

    bool state = manager->readMNESelFile(newPath, m_selectionGroupsMap);

    //Create group 'All' and 'All EEG' manually (bcause this group depends on the loaded channels from the fiff data file, not on the loaded selection file)
    m_selectionGroupsMap["All"] = m_currentlyLoadedFiffChannels;

    QStringList names;
//    for(int i = 0; i<m_pChInfoModel->rowCount(); i++) {
//        QModelIndex digIndex = m_pChInfoModel->index(i,1);
//        QString chName = m_pChInfoModel->data(digIndex,ChInfoModelRoles::GetOrigChName).toString();

//        digIndex = m_pChInfoModel->index(i,4);
//        int kind = m_pChInfoModel->data(digIndex,ChInfoModelRoles::GetChKind).toInt();

//        if(kind == FIFFV_EEG_CH) //FIFFV_MEG_CH
//            names<<chName;
//    }

    //Add 'Add EEG' group to selection groups
    m_selectionGroupsMap["All EEG"] = names;

    //Add selection groups to list widget
    QMapIterator<QString, QStringList> selectionIndex(m_selectionGroupsMap);
    while (selectionIndex.hasNext()) {
        selectionIndex.next();
        ui->m_listWidget_selectionGroups->insertItem(ui->m_listWidget_selectionGroups->count(), selectionIndex.key());
    }

    //Update selection
    updateSelectionGroupsList(getItemForChName(ui->m_listWidget_selectionGroups, "All"), new QListWidgetItem());

    //Set group all as slected item
    ui->m_listWidget_selectionGroups->setCurrentItem(getItemForChName(ui->m_listWidget_selectionGroups, "All"), QItemSelectionModel::Select);

    //Delete all MEG channels from the selection groups which are not in the loaded layout
    cleanUpMEGChannels();

    return state;
}


//*************************************************************************************************************

void SelectionManagerWindow::cleanUpMEGChannels()
{
    QMapIterator<QString,QStringList> selectionIndex(m_selectionGroupsMap);

    //Iterate through all loaded selection groups
    while (selectionIndex.hasNext()) {
        selectionIndex.next();

        QStringList channelList = selectionIndex.value();

        //Search the current selection group for MEG channels which are not in the currently loaded layout file and delete them
        QMutableStringListIterator stringListIndex(channelList);
        while (stringListIndex.hasNext()) {
            stringListIndex.next();

            if(!m_layoutMap.contains(stringListIndex.value()) && stringListIndex.value().contains("MEG"))
                stringListIndex.remove();
        }

        //Overwrite old selection groups channels
        m_selectionGroupsMap.insert(selectionIndex.key(), channelList);
    }
}


//*************************************************************************************************************

void SelectionManagerWindow::updateSelectionGroupsList(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if(current == 0)
        return;

    if(current->text().contains("EEG"))
        m_pSelectionScene->m_iChannelTypeMode = FIFFV_EEG_CH;
    else
        m_pSelectionScene->m_iChannelTypeMode = FIFFV_MEG_CH;

    ui->m_listWidget_visibleChannels->clear();

    //update visible channel list widget
    ui->m_listWidget_visibleChannels->addItems(m_selectionGroupsMap[current->text()]);

    //update scene items based o nthe new selection group
    updateSceneItems();

    //update the channels plotted in the data view
    updateDataView();
}


//*************************************************************************************************************

void SelectionManagerWindow::updateSceneItems()
{
    QStringList visibleItems;

    for(int i = 0; i<ui->m_listWidget_visibleChannels->count(); i++)
        visibleItems << ui->m_listWidget_visibleChannels->item(i)->text();

    m_pSelectionScene->hideItems(visibleItems);
}


//*************************************************************************************************************

void SelectionManagerWindow::updateUserDefinedChannelsList()
{
    QList<QGraphicsItem*> itemList = m_pSelectionScene->selectedItems();
    QStringList userDefinedChannels;

    for(int i = 0; i<itemList.size(); i++) {
        SelectionSceneItem* item = static_cast<SelectionSceneItem*>(itemList.at(i));
        userDefinedChannels << item->m_sChannelName;
    }

    ui->m_listWidget_userDefined->clear();
    ui->m_listWidget_userDefined->addItems(userDefinedChannels);

    updateDataView();
}


//*************************************************************************************************************

void SelectionManagerWindow::updateDataView()
{
    //if no channels have been selected by the user -> show selected group channels
    QListWidget* targetListWidget;
    if(ui->m_listWidget_userDefined->count()>0)
        targetListWidget = ui->m_listWidget_userDefined;
    else
        targetListWidget = ui->m_listWidget_visibleChannels;

    //Create list of channels which are to be visible in the view
    QStringList selectedChannels;

//    for(int i = 0; i<targetListWidget->count(); i++) {
//        QListWidgetItem* item = targetListWidget->item(i);
//        int indexTemp = m_pChInfoModel->getIndexFromMappedChName(item->text());

//        if(indexTemp != -1) {
//            QModelIndex mappedNameIndex = m_pChInfoModel->index(indexTemp,1);
//            QString origChName = m_pChInfoModel->data(mappedNameIndex,ChInfoModelRoles::GetOrigChName).toString();

//            selectedChannels << origChName;
//        }
//        else
//            selectedChannels << item->text();
//    }

    emit showSelectedChannelsOnly(selectedChannels);

    //emit signal that selection was changed
    if(!m_pSelectionScene->selectedItems().empty())
        emit selectionChanged(m_pSelectionScene->selectedItems());
    else
    {
        //only return visible items (EEG or MEG channels)
        QList<QGraphicsItem*> visibleItemList =  m_pSelectionScene->items();
        QMutableListIterator<QGraphicsItem*> i(visibleItemList);
        while (i.hasNext()) {
            if(!i.next()->isVisible())
                i.remove();
        }

        emit selectionChanged(visibleItemList);
    }
}


//*************************************************************************************************************

void SelectionManagerWindow::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);

    //Fit scene in view
    //ui->m_graphicsView_layoutPlot->fitInView(m_pSelectionScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

bool SelectionManagerWindow::eventFilter(QObject *obj, QEvent *event)
{
    //Setup delete key on user defined channel list
    if (obj == ui->m_listWidget_userDefined && event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if(keyEvent->key() == Qt::Key_Delete) {
            qDeleteAll(ui->m_listWidget_userDefined->selectedItems());
            updateDataView();
        }
        else
            return false;
    }
    else {
        // pass the event on to the parent class
        return QDockWidget::eventFilter(obj, event);
    }

    return false;
}
