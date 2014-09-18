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
    ui(new Ui::SelectionManagerWindow),
    m_pMainWindow(static_cast<MainWindow*>(parent))
{
    ui->setupUi(this);

    //Init gui elements
    initListWidgets();
    initGraphicsView();
}


//*************************************************************************************************************

SelectionManagerWindow::~SelectionManagerWindow()
{
    delete ui;
}


//*************************************************************************************************************

void SelectionManagerWindow::initListWidgets()
{
    //Initialise layout as neuromag vectorview with all channels
    loadLayout(":/Resources/Templates/Vectorview-grad.lout");

    //Initialise selections
    loadSelectionGroups(":/Resources/Templates/mne_browse_raw_vv.sel");

    connect(ui->m_listWidget_selectionFiles, &QListWidget::itemClicked,
                this, &SelectionManagerWindow::updateSelectionFiles);

    connect(ui->m_listWidget_selectionGroups, &QListWidget::itemClicked,
                this, &SelectionManagerWindow::updateSelectionGroups);

    ui->m_listWidget_selectedChannels->addItems(m_selectionGroups["Vertex"]);
}


//*************************************************************************************************************

void SelectionManagerWindow::initGraphicsView()
{
//    m_pLayoutScene = new LayoutScene(ui->m_graphicsView_layoutPlot);
//    ui->m_graphicsView_layoutPlot->setScene(m_pLayoutScene);
}


//*************************************************************************************************************

bool SelectionManagerWindow::loadLayout(QString path)
{
    LayoutLoader* manager = new LayoutLoader();

    return manager->readMNELoutFile(path, m_layoutMap);
}


//*************************************************************************************************************

bool SelectionManagerWindow::loadSelectionGroups(QString path)
{
    //Open .elc file
    if(!path.contains(".sel"))
        return false;

    m_selectionGroups.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening selection file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();

        if(line.contains("%") == false && line.contains(":") == true) //Skip commented areas in file
        {
            QStringList firstSplit = line.split(":");

            //Create new key
            QString key = firstSplit.at(0);

            QStringList secondSplit = firstSplit.at(1).split("|");

            //Delete last element if it is a blank character
            if(secondSplit.at(secondSplit.size()-1) == "")
                secondSplit.removeLast();

            //Add to map
            m_selectionGroups.insert(key, secondSplit);

            //Add to list widget
            int count = ui->m_listWidget_selectionGroups->count();
            if(count<1)
                ui->m_listWidget_selectionGroups->insertItem(0, key);
            else
                ui->m_listWidget_selectionGroups->insertItem(count, key);
        }
    }

    file.close();
}


//*************************************************************************************************************

void SelectionManagerWindow::updateSelectionFiles(QListWidgetItem* item)
{
    ui->m_listWidget_selectionGroups->clear();
    ui->m_listWidget_selectedChannels->clear();

    //update group list
    loadSelectionGroups(QString(":/Resources/Templates/%1.sel").arg(item->text()));
}


//*************************************************************************************************************

void SelectionManagerWindow::updateSelectionGroups(QListWidgetItem* item)
{
    ui->m_listWidget_selectedChannels->clear();

    //update channel list
    ui->m_listWidget_selectedChannels->addItems(m_selectionGroups[item->text()]);
}
