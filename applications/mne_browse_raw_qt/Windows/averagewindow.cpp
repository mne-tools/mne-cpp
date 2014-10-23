//=============================================================================================================
/**
* @file     averagewindow.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2014
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
* @brief    Contains the implementation of the AverageWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagewindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageWindow::AverageWindow(QWidget *parent, QFile &file)
: QDockWidget(parent)
, ui(new Ui::AverageWindow)
{
    ui->setupUi(this);

    initMVC(file);
    initTableViewWidgets();
    initAverageSceneView();
}


//*************************************************************************************************************

AverageWindow::~AverageWindow()
{
    delete ui;
}


//*************************************************************************************************************

AverageModel* AverageWindow::getAverageModel()
{
    return m_pAverageModel;
}


//*************************************************************************************************************

void AverageWindow::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    m_pAverageScene->repaintItems(selectedChannelItems);
}


//*************************************************************************************************************

void AverageWindow::initMVC(QFile &file)
{
    //Setup average model
    if(file.exists())
        m_pAverageModel = new AverageModel(file, this);
    else
        m_pAverageModel = new AverageModel(this);

    //Setup average delegate
    m_pAverageDelegate = new AverageDelegate(this);

    //Connect changes in the average data model to average delegate
//    connect(m_pAverageModel, &AverageModel::dataChanged,
//            this, &AverageWindow::onDataChanged);
}


//*************************************************************************************************************

void AverageWindow::initTableViewWidgets()
{
    //Set average model to list widget
    ui->m_tableView_loadedSets->setModel(m_pAverageModel);
    ui->m_tableView_loadedSets->setColumnHidden(1,true); //hide second column because the average model holds the aspect kind for this column
    ui->m_tableView_loadedSets->setColumnHidden(4,true); //hide last column because the average model holds the data types for this column
    ui->m_tableView_loadedSets->resizeColumnsToContents();
    ui->m_tableView_loadedSets->adjustSize();
    ui->m_tableView_loadedSets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    //Set initial selection
    ui->m_tableView_loadedSets->selectionModel()->select(QItemSelection(m_pAverageModel->index(0,0,QModelIndex()), m_pAverageModel->index(0,3,QModelIndex())),
                                                         QItemSelectionModel::Select);

    //Connect selection of the loaded evoked files
    connect(ui->m_tableView_loadedSets->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AverageWindow::onSelectionChanged);
}


//*************************************************************************************************************

void AverageWindow::initAverageSceneView()
{
    //Create average scene and set view
    m_pAverageScene = new AverageScene(ui->m_graphicsView_layout, this);
    ui->m_graphicsView_layout->setScene(m_pAverageScene);
}


//*************************************************************************************************************

void AverageWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Do for all selected evoked sets
    for(int u = 0; u<selected.indexes().size(); u++) {
        QModelIndex index = selected.indexes().at(u);

        //Get only the necessary data from the average model (use column 4)
        FiffInfo fiffInfo = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetFiffInfo).value<FiffInfo>();
        MatrixXd averageData = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetAverageData).value<MatrixXd>();
        int first = m_pAverageModel->data(m_pAverageModel->index(index.row(), 2), AverageModelRoles::GetFirstSample).value<MatrixXd>();
        int last = m_pAverageModel->data(m_pAverageModel->index(index.row(), 3), AverageModelRoles::GetLastSample).value<MatrixXd>();

        QStringList chNames = fiffInfo.ch_names;

        QList<VectorXd> channelAverageData;

        for(int i = 0; i<currentAverageSceneItems.size(); i++) {
            AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

            int channelNumber = chNames.indexOf(averageSceneItemTemp->m_sChannelName);
            if(channelNumber != -1) {
                averageSceneItemTemp->m_firstLastSample.first = first;
                averageSceneItemTemp->m_firstLastSample.last = last;
                averageSceneItemTemp->m_lAverageData.append(averageData.row(channelNumber));
            }
        }

        //Plot the data to the butterfly plot by providing the average delegate with the painter the butterfly scene
    }

}
