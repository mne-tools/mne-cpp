//=============================================================================================================
/**
 * @file     averagelayoutview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagelayoutview.h"

#include "helpers/averagescene.h"
#include "helpers/evokedsetmodel.h"
#include "helpers/channelinfomodel.h"
#include "helpers/averagesceneitem.h"

#include <fiff/fiff_info.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QSvgGenerator>
#include <QDebug>
#include <QGraphicsItem>
#include <QSettings>
#if !defined(NO_OPENGL)
    #include <QOpenGLWidget>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageLayoutView::AverageLayoutView(const QString& sSettingsPath,
                                     QWidget *parent,
                                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pFiffInfo(Q_NULLPTR)
, m_qMapAverageColor(QSharedPointer<QMap<QString, QColor> >::create())
, m_qMapAverageActivation(QSharedPointer<QMap<QString, bool> >::create())
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Average Layout");

    m_pAverageLayoutView = new QGraphicsView();

#if !defined(NO_OPENGL)
    m_pAverageLayoutView->setViewport(new QOpenGLWidget);
#endif

    m_pAverageLayoutView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pAverageLayoutView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pAverageScene = AverageScene::SPtr(new AverageScene(m_pAverageLayoutView.data(), this));
    m_pAverageScene->setBackgroundBrush(QBrush(Qt::white));

    m_pAverageLayoutView->setScene(m_pAverageScene.data());

    //set layouts
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0,0,0,0);
    neLayout->addWidget(m_pAverageLayoutView);
    this->setLayout(neLayout);
    loadSettings();
}

//=============================================================================================================

AverageLayoutView::~AverageLayoutView()
{
    saveSettings();
}

//=============================================================================================================

void AverageLayoutView::updateOpenGLViewport()
{
#if !defined(NO_OPENGL)
    if(m_pAverageLayoutView) {
        m_pAverageLayoutView->setViewport(new QOpenGLWidget);
    }
#endif
}

//=============================================================================================================

void AverageLayoutView::setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel)
{
    m_pChannelInfoModel = pChannelInfoModel;
}

//=============================================================================================================

void AverageLayoutView::setEvokedSetModel(QSharedPointer<EvokedSetModel> &pEvokedSetModel)
{
    connect(pEvokedSetModel.data(), &EvokedSetModel::dataChanged,
            this, &AverageLayoutView::updateData);

    m_pEvokedSetModel = pEvokedSetModel;
}

//=============================================================================================================

void AverageLayoutView::setBackgroundColor(const QColor& backgroundColor)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::setBackgroundColor - m_pAverageScene is NULL. Returning. ";
        return;
    }

    QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
    backgroundBrush.setColor(backgroundColor);
    m_pAverageScene->setBackgroundBrush(backgroundBrush);
}

//=============================================================================================================

QColor AverageLayoutView::getBackgroundColor()
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::getBackgroundColor - m_pAverageScene is NULL. Returning. ";
        return QColor();
    }

    return m_pAverageScene->backgroundBrush().color();
}

//=============================================================================================================

void AverageLayoutView::takeScreenshot(const QString& fileName)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::takeScreenshot - m_pAverageScene is NULL. Returning. ";
        return;
    }

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
        QPixmap pixMap = QPixmap::grabWidget(m_pAverageLayoutView);
        pixMap.save(fileName);
    }
}

//=============================================================================================================

void AverageLayoutView::setScaleMap(const QMap<qint32,float> &scaleMap)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::setScaleMap - m_pAverageScene is NULL. Returning. ";
        return;
    }

    m_pAverageScene->setScaleMap(scaleMap);

    updateData();
}

//=============================================================================================================

QSharedPointer<QMap<QString, QColor> > AverageLayoutView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

QSharedPointer<QMap<QString, bool> > AverageLayoutView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void AverageLayoutView::setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::setAverageColor - m_pAverageScene is NULL. Returning. ";
        return;
    }

    m_qMapAverageColor = qMapAverageColor;
    m_pAverageScene->setColorPerAverage(m_qMapAverageColor);
}

//=============================================================================================================

void AverageLayoutView::setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::setAverageActivation - m_pAverageScene is NULL. Returning. ";
        return;
    }

    m_qMapAverageActivation = qMapAverageActivation;
    m_pAverageScene->setActivationPerAverage(qMapAverageActivation);
}

//=============================================================================================================

void AverageLayoutView::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    if(!m_pAverageScene) {
        qDebug() << "AverageLayoutView::channelSelectionManagerChanged - m_pAverageScene is NULL. Returning. ";
        return;
    }
    std::cout<<"A" <<std::endl;
    //Repaint the average items in the average scene based on the input parameter selectedChannelItems and update them with current data
    m_pAverageScene->repaintItems(selectedChannelItems);
    std::cout<<"B" <<std::endl;
    setAverageColor(m_qMapAverageColor);
    std::cout<<"C" <<std::endl;
    setAverageActivation(m_qMapAverageActivation);
    std::cout<<"D" <<std::endl;
    setScaleMap(m_scaleMap);
    std::cout<<"E" <<std::endl;
    updateData();
    std::cout<<"F" <<std::endl;
}

//=============================================================================================================

void AverageLayoutView::updateData()
{
    std::cout <<"1" << std::endl;
    if(m_pFiffInfo) {
    std::cout <<"2" << std::endl;
        if (m_listMappedChannelNames.isEmpty()){
            return;
        }
//        GetChKind m_pFiffInfo->chs.at(index.row()).kind;
//        GetChUnit m_pFiffInfo->chs.at(index.row()).unit;
        std::cout << "name list size: " << m_listMappedChannelNames.size() << std::endl;
        QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();
        std::cout <<"3" << std::endl;
        //Set new data for all averageSceneItems
        for(int i = 0; i < currentAverageSceneItems.size(); i++) {
            AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

            averageSceneItemTemp->m_lAverageData.clear();

            //Get only the necessary data from the average model (use column 2)
            QList<QPair<QString, DISPLIB::RowVectorPair> > averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<QList<QPair<QString, DISPLIB::RowVectorPair> > >();

            //Get the averageScenItem specific data row
            //int channelNumber = m_pChannelInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName);
            int channelNumber = m_listMappedChannelNames.indexOf(averageSceneItemTemp->m_sChannelName);

            std::cout << "chan number:" << channelNumber << " - name:" << averageSceneItemTemp->m_sChannelName.toStdString() << std::endl;
            std::cout << "namelist" << m_listMappedChannelNames.first().toStdString() << std::endl;

            if(channelNumber != -1) {
                //qDebug() << "Change data for" << channelNumber << "" << averageSceneItemTemp->m_sChannelName;

                //averageSceneItemTemp->m_iChannelKind = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 4), ChannelInfoModelRoles::GetChKind).toInt();
                averageSceneItemTemp->m_iChannelKind = m_pFiffInfo->chs.at(channelNumber).kind;
                //averageSceneItemTemp->m_iChannelUnit = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 6), ChannelInfoModelRoles::GetChUnit).toInt();
                averageSceneItemTemp->m_iChannelUnit = m_pFiffInfo->chs.at(channelNumber).unit;
                averageSceneItemTemp->m_firstLastSample.first = (-1)*m_pEvokedSetModel->getNumPreStimSamples();

                if(!averageData.isEmpty()) {
                    averageSceneItemTemp->m_firstLastSample.second = averageData.first().second.second - m_pEvokedSetModel->getNumPreStimSamples();
                }

                averageSceneItemTemp->m_iChannelNumber = channelNumber;
                averageSceneItemTemp->m_iTotalNumberChannels = m_pEvokedSetModel->rowCount();
                averageSceneItemTemp->m_lAverageData = averageData;
                averageSceneItemTemp->m_bIsBad = m_pEvokedSetModel->getIsChannelBad(channelNumber);
            }
        }

        m_pAverageScene->updateScene();
        std::cout <<"4" << std::endl;
        return;
    }

    if(!m_pAverageScene || !m_pEvokedSetModel || !m_pChannelInfoModel) {
        std::cout <<"5" << std::endl;
        qDebug() << "AverageLayoutView::updateData - m_pAverageScene, m_pEvokedSetModel or m_pChannelInfoModel are NULL. Returning. ";
        return;
    }
    std::cout <<"6" << std::endl;
    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i < currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Get only the necessary data from the average model (use column 2)
        QList<QPair<QString, DISPLIB::RowVectorPair> > averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<QList<QPair<QString, DISPLIB::RowVectorPair> > >();

        //Get the averageScenItem specific data row
        int channelNumber = m_pChannelInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName);
        std::cout << "chan number:" << channelNumber << " - name:" << averageSceneItemTemp->m_sChannelName.toStdString() << std::endl;

        if(channelNumber != -1) {
            //qDebug() << "Change data for" << channelNumber << "" << averageSceneItemTemp->m_sChannelName;

            averageSceneItemTemp->m_iChannelKind = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 4), ChannelInfoModelRoles::GetChKind).toInt();
            averageSceneItemTemp->m_iChannelUnit = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 6), ChannelInfoModelRoles::GetChUnit).toInt();
            averageSceneItemTemp->m_firstLastSample.first = (-1)*m_pEvokedSetModel->getNumPreStimSamples();

            if(!averageData.isEmpty()) {
                averageSceneItemTemp->m_firstLastSample.second = averageData.first().second.second - m_pEvokedSetModel->getNumPreStimSamples();
            }

            averageSceneItemTemp->m_iChannelNumber = channelNumber;
            averageSceneItemTemp->m_iTotalNumberChannels = m_pEvokedSetModel->rowCount();
            averageSceneItemTemp->m_lAverageData = averageData;
            averageSceneItemTemp->m_bIsBad = m_pEvokedSetModel->getIsChannelBad(channelNumber);
        }
    }

    m_pAverageScene->updateScene();
}

//=============================================================================================================

void AverageLayoutView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void AverageLayoutView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void AverageLayoutView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AverageLayoutView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void AverageLayoutView::setFiffInfo(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}

//=============================================================================================================

void AverageLayoutView::setMappedChannelNames(const QStringList &mappedLayoutChNames)
{
    std::cout << "AverageLayoutView::setMappedChannelNames";
    m_listMappedChannelNames = mappedLayoutChNames;
}
