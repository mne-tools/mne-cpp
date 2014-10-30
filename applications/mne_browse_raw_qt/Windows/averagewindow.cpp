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
    initButtons();
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

void AverageWindow::scaleAveragedData(const QMap<QString,double> &scaleMap)
{
    m_pAverageScene->setScaleMap(scaleMap);
    m_pButterflyScene->setScaleMap(scaleMap);
}


//*************************************************************************************************************

void AverageWindow::initButtons()
{
    connect(ui->m_pushButton_exportLayoutPlot, &QPushButton::released,
            this, &AverageWindow::exportAverageLayoutPlot);
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

    //Create butterfly average scene and set view
    m_pButterflyScene = new ButterflyScene(ui->m_graphicsView_butterflyPlot, this);
    ui->m_graphicsView_butterflyPlot->setScene(m_pButterflyScene);
}


//*************************************************************************************************************

void AverageWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    Q_UNUSED(selected);

    qDebug()<<"AverageWindow::onSelectionChanged";
    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Do for all selected evoked sets
        for(int u = 0; u<selected.indexes().size(); u++) {
            //Get only the necessary data from the average model (use column 4)
            QModelIndex index = selected.indexes().at(u);

            const FiffInfo* fiffInfo = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetFiffInfo).value<const FiffInfo*>();
            RowVectorPair averageData = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetAverageData).value<RowVectorPair>();
            int first = m_pAverageModel->data(m_pAverageModel->index(index.row(), 2), AverageModelRoles::GetFirstSample).toInt();
            int last = m_pAverageModel->data(m_pAverageModel->index(index.row(), 3), AverageModelRoles::GetLastSample).toInt();

            //Get the averageScenItem specific data row
            QStringList chNames = fiffInfo->ch_names;

            int channelNumber = chNames.indexOf(averageSceneItemTemp->m_sChannelName);
            if(channelNumber != -1) {
                averageSceneItemTemp->m_firstLastSample.first = first;
                averageSceneItemTemp->m_firstLastSample.second = last;
                averageSceneItemTemp->m_iChannelKind = fiffInfo->chs.at(channelNumber).kind;
                averageSceneItemTemp->m_iChannelUnit = fiffInfo->chs.at(channelNumber).unit;;
                averageSceneItemTemp->m_iChannelNumber = channelNumber;
                averageSceneItemTemp->m_iTotalNumberChannels = chNames.size();
                averageSceneItemTemp->m_lAverageData.append(averageData);
            }
        }
    }

    m_pAverageScene->update();
    ui->m_graphicsView_layout->fitInView(m_pAverageScene->itemsBoundingRect(), Qt::KeepAspectRatio);

    //Draw butterfly plot
    m_pButterflyScene->clear();

    for(int i = 0; i<selected.indexes().size(); i++) {
        //Get only the necessary data from the average model (use column 4)
        QModelIndex index = selected.indexes().at(i);

        const FiffInfo* fiffInfo = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetFiffInfo).value<const FiffInfo*>();
        RowVectorPair averageData = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetAverageData).value<RowVectorPair>();
        int first = m_pAverageModel->data(m_pAverageModel->index(index.row(), 2), AverageModelRoles::GetFirstSample).toInt();
        int last = m_pAverageModel->data(m_pAverageModel->index(index.row(), 3), AverageModelRoles::GetLastSample).toInt();
        QString setName = m_pAverageModel->data(m_pAverageModel->index(index.row(), 0), Qt::DisplayRole).toString();

        //Generate random colors
        QList<QColor> butterflyColors;
        for(int i = 0; i<fiffInfo->chs.size(); i++)
            butterflyColors.append(QColor(qrand()%256, qrand()%256, qrand()%256));

        //Create new butterfly scene item
        ButterflySceneItem* butterflySceneItemTemp = new ButterflySceneItem(setName, FIFFV_MEG_CH, butterflyColors); //TODO: let the user decide , EEG, MEG_Grad, MEG_mag

        butterflySceneItemTemp->m_lAverageData.first = averageData.first;
        butterflySceneItemTemp->m_lAverageData.second = averageData.second;
        butterflySceneItemTemp->m_pFiffInfo = fiffInfo;
        butterflySceneItemTemp->m_firstLastSample.first = first;
        butterflySceneItemTemp->m_firstLastSample.second = last;

        m_pButterflyScene->addItem(butterflySceneItemTemp);
    }

    m_pButterflyScene->update();
    ui->m_graphicsView_butterflyPlot->fitInView(m_pButterflyScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void AverageWindow::exportAverageLayoutPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save average plot",
                                                    QString("%1/%2_%3_%4_AveragePlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pAverageScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pAverageScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pAverageScene->setSceneRect(m_pAverageScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pAverageScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pAverageScene->render(&painter);
            image.save(fileName);
        }
    }
}
