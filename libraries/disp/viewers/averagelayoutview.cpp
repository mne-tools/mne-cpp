//=============================================================================================================
/**
* @file     averagelayoutview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the AverageLayoutView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagelayoutview.h"

#include "helpers/averagescene.h"
#include "helpers/evokedsetmodel.h"
#include "helpers/chinfomodel.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QSvgGenerator>



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageLayoutView::AverageLayoutView(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    this->setWindowTitle("Average Layout");

    m_pAverageLayoutView = new QGraphicsView(this);
    m_pAverageScene = AverageScene::SPtr(new AverageScene(m_pAverageLayoutView.data(), this));

    m_pAverageLayoutView->setScene(m_pAverageScene.data());
    QBrush brush(Qt::black);
    m_pAverageScene->setBackgroundBrush(brush);

    //set layouts
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pAverageLayoutView.data());
    this->setLayout(neLayout);
}


//*************************************************************************************************************

void AverageLayoutView::setFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> &pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}


//*************************************************************************************************************

void AverageLayoutView::setChInfoModel(QSharedPointer<ChInfoModel> &pChInfoModel)
{
    m_pChInfoModel = pChInfoModel;
}


//*************************************************************************************************************

void AverageLayoutView::setEvokedSetModel(QSharedPointer<EvokedSetModel> &pEvokedSetModel)
{
    m_pEvokedSetModel = pEvokedSetModel;
}


//*************************************************************************************************************

void AverageLayoutView::setBackgroundColor(const QColor& backgroundColor)
{
    QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
    backgroundBrush.setColor(backgroundColor);
    m_pAverageScene->setBackgroundBrush(backgroundBrush);
}


//*************************************************************************************************************

QColor AverageLayoutView::getBackgroundColor()
{
    return m_pAverageScene->backgroundBrush().color();
}


//*************************************************************************************************************

void AverageLayoutView::takeScreenshot(const QString& fileName)
{
    if(fileName.contains(".svg", Qt::CaseInsensitive))
    {
        // Generate screenshot
        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        QRectF rect = m_pAverageScene->itemsBoundingRect();
        svgGen.setSize(QSize(rect.width(), rect.height()));

        QPainter painter(&svgGen);
        m_pAverageScene->render(&painter);
    }

    if(fileName.contains(".png", Qt::CaseInsensitive))
    {
        QPixmap pixMap = QPixmap::grabWidget(m_pAverageLayoutView.data());
        pixMap.save(fileName);
    }
}


//*************************************************************************************************************

void AverageLayoutView::setScaleMap(const QMap<qint32,float> &scaleMap)
{
    m_pAverageScene->setScaleMap(scaleMap);
}


//*************************************************************************************************************

void AverageLayoutView::setAverageInformationMap(const QMap<double, QPair<QColor, QPair<QString,bool> > >& mapAvr)
{
    m_averageInfos = mapAvr;
    m_pAverageScene->setAverageInformationMap(mapAvr);
}


//*************************************************************************************************************

void AverageLayoutView::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    //Repaint the average items in the average scene based on the input parameter selectedChannelItems and update them with current data
    m_pAverageScene->repaintItems(selectedChannelItems);
    setAverageInformationMap(m_averageInfos);
    updateData();
}


//*************************************************************************************************************

void AverageLayoutView::updateData()
{
    if(!m_pEvokedSetModel || !m_pChInfoModel || !m_pFiffInfo) {
        qDebug() << "AverageLayoutView::updateData - m_pEvokedSetModel, m_pChInfoModel or m_pFiffInfo are NULL. Returning. ";
        return;
    }

    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Get only the necessary data from the average model (use column 2)
        QList<QPair<double, DISPLIB::RowVectorPair> > averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<QList<QPair<double, DISPLIB::RowVectorPair> > >();

        //Get the averageScenItem specific data row
        int channelNumber = m_pChInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName);

        if(channelNumber != -1) {
            //qDebug() << "Change data for" << channelNumber << "" << averageSceneItemTemp->m_sChannelName;
            averageSceneItemTemp->m_iChannelKind = m_pFiffInfo->chs.at(channelNumber).kind;
            averageSceneItemTemp->m_iChannelUnit = m_pFiffInfo->chs.at(channelNumber).unit;
            averageSceneItemTemp->m_firstLastSample.first = (-1)*m_pEvokedSetModel->getNumPreStimSamples();

            if(!averageData.isEmpty()) {
                averageSceneItemTemp->m_firstLastSample.second = averageData.first().second.second - m_pEvokedSetModel->getNumPreStimSamples();
            }

            averageSceneItemTemp->m_iChannelNumber = channelNumber;
            averageSceneItemTemp->m_iTotalNumberChannels = m_pFiffInfo->ch_names.size();
            averageSceneItemTemp->m_lAverageData = averageData;
        }
    }

    m_pAverageScene->updateScene();
}

