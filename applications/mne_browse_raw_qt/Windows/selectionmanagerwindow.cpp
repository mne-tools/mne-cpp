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

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionManagerWindow::SelectionManagerWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SelectionManagerWindow)
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

void SelectionManagerWindow::setCurrentlyLoadedFiffChannels(FiffInfo loadedFiffInfo)
{
    QStringList loadedFiffChannels;

    for(int i = 0; i<loadedFiffInfo.chs.size() ; i++)
        loadedFiffChannels<<loadedFiffInfo.chs.at(i).ch_name;

    m_currentlyLoadedFiffChannels = loadedFiffChannels;

    loadLayout(ui->m_comboBox_layoutFile->currentText());
}


//*************************************************************************************************************

void SelectionManagerWindow::highlightChannels(QStringList channelList)
{
    QList<QGraphicsItem *> allSceneItems = m_pSelectionScene->items();

    for(int i = 0; i<allSceneItems.size(); i++) {
        ChannelSceneItem* item = static_cast<ChannelSceneItem*>(allSceneItems.at(i));
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
        ChannelSceneItem* item = static_cast<ChannelSceneItem*>(allSceneItems.at(i));
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

QListWidgetItem* SelectionManagerWindow::getItem(QListWidget* listWidget, QString channelName)
{
    for(int i=0; i<listWidget->count(); i++)
        if(listWidget->item(i)->text() == channelName)
            return listWidget->item(i);

    return new QListWidgetItem();
}


//*************************************************************************************************************

void SelectionManagerWindow::initListWidgets()
{
    //Install event filter to receive key press events
    ui->m_listWidget_userDefined->installEventFilter(this);

    //Connect list widgets to update themselves and other list widgets when changed
    connect(ui->m_listWidget_selectionGroups, &QListWidget::itemClicked,
                this, &SelectionManagerWindow::updateSelectionGroups);

    connect(ui->m_listWidget_selectionGroups, &QListWidget::itemClicked,
                this, &SelectionManagerWindow::updateSceneItems);

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
                this, &SelectionManagerWindow::updateUserDefinedChannels);
}


//*************************************************************************************************************

void SelectionManagerWindow::initComboBoxes()
{
    //Connect the layout and selection group loader
    connect(ui->m_comboBox_selectionFiles, &QComboBox::currentTextChanged,
                this, &SelectionManagerWindow::loadSelectionGroups);

    connect(ui->m_comboBox_layoutFile, &QComboBox::currentTextChanged,
                this, &SelectionManagerWindow::loadLayout);

    //Initialise layout as neuromag vectorview with all channels
    loadLayout("Vectorview-all.lout");
}


//*************************************************************************************************************

bool SelectionManagerWindow::loadLayout(QString path)
{
    //Read layout
    LayoutLoader* manager = new LayoutLoader();
    QString newPath = QCoreApplication::applicationDirPath() + path.prepend("/MNE_Browse_Raw_Resources/Templates/Layouts/");

    bool state = manager->readMNELoutFile(newPath, m_layoutMap);

    //Load selection groups again because they need to be reinitialised every time a new layout hase been loaded
    loadSelectionGroups(ui->m_comboBox_selectionFiles->currentText());

    //Update scene
    m_pSelectionScene->repaintItems(m_layoutMap);
    m_pSelectionScene->update();

    //Fit to view
    ui->m_graphicsView_layoutPlot->fitInView(m_pSelectionScene->itemsBoundingRect(), Qt::KeepAspectRatio);

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

    //Create group 'All' manually (bcause this group depends on the loaded channels from the fiff data file, not on the loaded selection file)
    m_selectionGroupsMap["All"] = m_currentlyLoadedFiffChannels;

    //Add selection groups to list widget
    QMapIterator<QString, QStringList> selectionIndex(m_selectionGroupsMap);
    while (selectionIndex.hasNext()) {
        selectionIndex.next();
        ui->m_listWidget_selectionGroups->insertItem(ui->m_listWidget_selectionGroups->count(), selectionIndex.key());
    }

    //Delete all MEG channels from the selection groups which are not in the loaded layout
    cleanUpMEGChannels();

    //Set group all as slected item
    ui->m_listWidget_selectionGroups->setCurrentItem(getItem(ui->m_listWidget_selectionGroups, "All"), QItemSelectionModel::Select);

    //Update selection
    updateSelectionGroups(getItem(ui->m_listWidget_selectionGroups, "All"));

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

void SelectionManagerWindow::updateSelectionGroups(QListWidgetItem* item)
{
    ui->m_listWidget_visibleChannels->clear();

    //update channel list
    ui->m_listWidget_visibleChannels->addItems(m_selectionGroupsMap[item->text()]);

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

void SelectionManagerWindow::updateUserDefinedChannels()
{
    QList<QGraphicsItem*> itemList = m_pSelectionScene->selectedItems();
    QStringList userDefinedChannels;

    for(int i = 0; i<itemList.size(); i++) {
        ChannelSceneItem* item = static_cast<ChannelSceneItem*>(itemList.at(i));
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

    for(int i = 0; i<targetListWidget->count(); i++) {
        QListWidgetItem* item = targetListWidget->item(i);
        selectedChannels << item->text();
    }

    emit showSelectedChannelsOnly(selectedChannels);

    //emit signal that selection was changed

    if(!m_pSelectionScene->selectedItems().empty())
        emit selectionChanged(m_pSelectionScene->selectedItems());
    else
        emit selectionChanged(m_pSelectionScene->items());
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
    //Setup delte key on user defined channel list
    if (obj == ui->m_listWidget_userDefined) {
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
