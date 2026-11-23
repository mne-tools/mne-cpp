//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     averagelayoutview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Implementation of the AverageLayoutView sensor-layout evoked viewer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagelayoutview.h"
#include <mne/base/log.h>

#include "helpers/averagescene.h"
#include "helpers/evokedsetmodel.h"
#include "helpers/channelinfomodel.h"
#include "helpers/averagesceneitem.h"
#include "helpers/selectionsceneitem.h"
#include "../plots/helpers/svgwriter.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QPainter>
#include <QGraphicsItem>
#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageLayoutView::AverageLayoutView(const QString& sSettingsPath,
                                     QWidget* parent,
                                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_qMapAverageColor(std::make_shared<std::map<std::string, QColor>>())
, m_qMapAverageActivation(std::make_shared<std::map<std::string, bool>>())
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Average Layout");

    m_pAverageLayoutView = new QGraphicsView();


    m_pAverageLayoutView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pAverageLayoutView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pAverageScene = std::make_shared<AverageScene>(m_pAverageLayoutView.data(), this);
    m_pAverageScene->setBackgroundBrush(QBrush(Qt::white));

    m_pAverageLayoutView->setScene(m_pAverageScene.get());

    //set layouts
    QVBoxLayout* neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0, 0, 0, 0);
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

void AverageLayoutView::updateViewport()
{
    // The scene items paint with QPainter, so the viewport stays Qt's raster widget
    if (m_pAverageLayoutView) {
        m_pAverageLayoutView->viewport()->update();
    }
}

//=============================================================================================================

void AverageLayoutView::setChannelInfoModel(std::shared_ptr<ChannelInfoModel>& pChannelInfoModel)
{
    m_pChannelInfoModel = pChannelInfoModel;
}

//=============================================================================================================

void AverageLayoutView::setEvokedSetModel(const std::shared_ptr<EvokedSetModel>& pEvokedSetModel)
{
    if (pEvokedSetModel) {
        connect(pEvokedSetModel.get(), &EvokedSetModel::dataChanged,
                this, &AverageLayoutView::updateData, Qt::UniqueConnection);
    }

    m_pEvokedSetModel = pEvokedSetModel;
}

//=============================================================================================================

void AverageLayoutView::setBackgroundColor(const QColor& backgroundColor)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::setBackgroundColor - m_pAverageScene is NULL. Returning. ");
        return;
    }

    QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
    backgroundBrush.setColor(backgroundColor);
    m_pAverageScene->setBackgroundBrush(backgroundBrush);
}

//=============================================================================================================

QColor AverageLayoutView::getBackgroundColor()
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::getBackgroundColor - m_pAverageScene is NULL. Returning. ");
        return QColor();
    }

    return m_pAverageScene->backgroundBrush().color();
}

//=============================================================================================================

void AverageLayoutView::takeScreenshot(const QString& fileName)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::takeScreenshot - m_pAverageScene is NULL. Returning. ");
        return;
    }

    if (fileName.contains(".svg", Qt::CaseInsensitive)) {
        // Generate screenshot
        SvgWriter svgGen;
        svgGen.setFileName(fileName.toStdString());
        QRectF rect = m_pAverageScene->itemsBoundingRect();
        svgGen.setSize(QSize(rect.width(), rect.height()));

        QPainter painter(&svgGen);
        m_pAverageScene->render(&painter);
    }

    if (fileName.contains(".png", Qt::CaseInsensitive)) {
        QRect rect = m_pAverageLayoutView->frameRect();
        QPixmap pixMap = QWidget::grab(rect);
        pixMap.save(fileName);
    }
}

//=============================================================================================================

void AverageLayoutView::setScaleMap(const std::map<int, float>& scaleMap)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::setScaleMap - m_pAverageScene is NULL. Returning. ");
        return;
    }

    m_scaleMap = scaleMap;

    m_pAverageScene->setScaleMap(scaleMap);

    updateData();
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, QColor>> AverageLayoutView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, bool>> AverageLayoutView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void AverageLayoutView::setAverageColor(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::setAverageColor - m_pAverageScene is NULL. Returning. ");
        return;
    }

    m_qMapAverageColor = qMapAverageColor;
    m_pAverageScene->setColorPerAverage(m_qMapAverageColor);
}

//=============================================================================================================

void AverageLayoutView::setSingleAverageColor(const QColor& avgColor)
{
    for (auto& [key, color] : *m_qMapAverageColor) {
        color = avgColor;
    }

    m_pAverageScene->setColorPerAverage(m_qMapAverageColor);
    m_pAverageScene->setSignalItemColor(avgColor);
}

//=============================================================================================================

void AverageLayoutView::setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::setAverageActivation - m_pAverageScene is NULL. Returning. ");
        return;
    }

    m_qMapAverageActivation = qMapAverageActivation;
    m_pAverageScene->setActivationPerAverage(qMapAverageActivation);
}

//=============================================================================================================

void AverageLayoutView::channelSelectionManagerChanged(const QList<QGraphicsItem*>& selectedChannelItems)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::channelSelectionManagerChanged - m_pAverageScene is NULL. Returning. ");
        return;
    }
    //Repaint the average items in the average scene based on the input parameter selectedChannelItems and update them with current data
    m_pAverageScene->repaintItems(selectedChannelItems);
    setAverageColor(m_qMapAverageColor);
    setAverageActivation(m_qMapAverageActivation);
    setScaleMap(m_scaleMap);
    updateData();
}

//=============================================================================================================

void AverageLayoutView::channelSelectionChanged(const QVariant& selectionData)
{
    if (!m_pAverageScene) {
        mne::Log::debug("AverageLayoutView::channelSelectionManagerChanged - m_pAverageScene is NULL. Returning. ");
        return;
    }

    SelectionItem* pSelectionItem = selectionData.value<mne::disp::SelectionItem*>();

    //Repaint the average items in the average scene based on the input parameter selectedChannelItems and update them with current selectionData
    m_pAverageScene->repaintSelectionItems(*pSelectionItem);
    setAverageColor(m_qMapAverageColor);
    setAverageActivation(m_qMapAverageActivation);
    setScaleMap(m_scaleMap);
    updateData();
}

//=============================================================================================================

void AverageLayoutView::updateData()
{
    if (m_pFiffInfo) {
        QList<QGraphicsItem*> currentAverageSceneItems = m_pAverageScene->items();

        //Set new data for all averageSceneItems
        for (int i = 0; i < currentAverageSceneItems.size(); i++) {
            AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

            averageSceneItemTemp->m_lAverageData.clear();

            if (m_pEvokedSetModel) {
                //Get only the necessary data from the average model (use column 2)
                const std::vector<mne::disp::AvrTypeRowVectorPair> averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<std::vector<mne::disp::AvrTypeRowVectorPair>>();

                //Get the averageScenItem specific data row
                int channelNumber = averageSceneItemTemp->m_iChannelNumber;

                if (channelNumber != -1) {
                    averageSceneItemTemp->m_iChannelKind = m_pFiffInfo->chs.at(channelNumber).kind;
                    averageSceneItemTemp->m_iChannelUnit = m_pFiffInfo->chs.at(channelNumber).unit;
                    averageSceneItemTemp->m_firstLastSample.first = (-1) * m_pEvokedSetModel->getNumPreStimSamples();

                    if (!averageData.empty()) {
                        averageSceneItemTemp->m_firstLastSample.second = averageData.front().second.second - m_pEvokedSetModel->getNumPreStimSamples();
                    }

                    averageSceneItemTemp->m_iTotalNumberChannels = m_pEvokedSetModel->rowCount();
                    averageSceneItemTemp->m_lAverageData = averageData;
                    averageSceneItemTemp->m_bIsBad = m_pEvokedSetModel->getIsChannelBad(channelNumber);
                }
            }
        }

        m_pAverageScene->updateScene();
        return;
    }

    if (!m_pAverageScene || !m_pEvokedSetModel || !m_pChannelInfoModel) {
        mne::Log::debug("AverageLayoutView::updateData - m_pAverageScene, m_pEvokedSetModel or m_pChannelInfoModel are NULL. Returning. ");
        return;
    }
    //Get current items from the average scene
    QList<QGraphicsItem*> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for (int i = 0; i < currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Get only the necessary data from the average model (use column 2)
        const std::vector<mne::disp::AvrTypeRowVectorPair> averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<std::vector<mne::disp::AvrTypeRowVectorPair>>();

        //Get the averageScenItem specific data row
        int channelNumber = m_pChannelInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName.toStdString());

        if (channelNumber != -1) {
            //qDebug() << "Change data for" << channelNumber << "" << averageSceneItemTemp->m_sChannelName;

            averageSceneItemTemp->m_iChannelKind = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 4), ChannelInfoModelRoles::GetChKind).toInt();
            averageSceneItemTemp->m_iChannelUnit = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 6), ChannelInfoModelRoles::GetChUnit).toInt();
            averageSceneItemTemp->m_firstLastSample.first = (-1) * m_pEvokedSetModel->getNumPreStimSamples();

            if (!averageData.empty()) {
                averageSceneItemTemp->m_firstLastSample.second = averageData.front().second.second - m_pEvokedSetModel->getNumPreStimSamples();
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
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void AverageLayoutView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void AverageLayoutView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AverageLayoutView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void AverageLayoutView::setFiffInfo(const std::shared_ptr<mne::FiffInfo>& pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}

//=============================================================================================================

std::shared_ptr<EvokedSetModel> AverageLayoutView::getEvokedSetModel()
{
    return m_pEvokedSetModel;
}

//=============================================================================================================

void AverageLayoutView::clearView()
{
    setEvokedSetModel(nullptr);
    updateData();
}
