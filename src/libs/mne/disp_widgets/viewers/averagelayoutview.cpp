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
#include "helpers/averagelayoutwidget.h"
#include <mne/disp/viewers/helpers/channelinfomodel.h>
#include <mne/disp/viewers/helpers/evokedsetmodel.h>
#include <mne/disp/viewers/helpers/selectionitem.h>
#include <mne/base/log.h>
#include <mne/fiff/fiff_info.h>
#include <mne/scene/svg_renderer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPixmap>
#include <QSettings>
#include <QVBoxLayout>

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

    m_pAverageLayoutWidget = new AverageLayoutWidget(this);

    QVBoxLayout* neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0, 0, 0, 0);
    neLayout->addWidget(m_pAverageLayoutWidget);
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
    if (m_pAverageLayoutWidget) {
        m_pAverageLayoutWidget->update();
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
    m_pAverageLayoutWidget->setBackgroundColor(backgroundColor);
}

//=============================================================================================================

QColor AverageLayoutView::getBackgroundColor()
{
    return m_pAverageLayoutWidget->backgroundColor();
}

//=============================================================================================================

void AverageLayoutView::takeScreenshot(const QString& fileName)
{
    if (fileName.contains(".svg", Qt::CaseInsensitive)) {
        // The tiles as vector graphics, at the size they have on screen
        const scene::DisplayList figure = m_pAverageLayoutWidget->figure(m_pAverageLayoutWidget->size());
        if (const auto written = scene::write_svg(figure, fileName.toStdU16String()); !written) {
            mne::Log::warning("{}", written.error().what());
        }
    }

    if (fileName.contains(".png", Qt::CaseInsensitive)) {
        m_pAverageLayoutWidget->grab().save(fileName);
    }
}

//=============================================================================================================

void AverageLayoutView::setScaleMap(const std::map<int, float>& scaleMap)
{
    m_scaleMap = scaleMap;
    m_pAverageLayoutWidget->setScaleMap(scaleMap);
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
    m_qMapAverageColor = qMapAverageColor;
    m_pAverageLayoutWidget->setAverageColors(*m_qMapAverageColor);
}

//=============================================================================================================

void AverageLayoutView::setSingleAverageColor(const QColor& avgColor)
{
    for (auto& [key, color] : *m_qMapAverageColor) {
        color = avgColor;
    }

    m_pAverageLayoutWidget->setAverageColors(*m_qMapAverageColor);
    m_pAverageLayoutWidget->setSignalColor(avgColor);
}

//=============================================================================================================

void AverageLayoutView::setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    m_pAverageLayoutWidget->setAverageActivation(*m_qMapAverageActivation);
}

//=============================================================================================================

void AverageLayoutView::channelSelectionManagerChanged(const SelectionItem& selection)
{
    m_pAverageLayoutWidget->setChannels(AverageLayoutWidget::channelsFrom(selection));
    setAverageColor(m_qMapAverageColor);
    setAverageActivation(m_qMapAverageActivation);
    setScaleMap(m_scaleMap);
}

//=============================================================================================================

void AverageLayoutView::channelSelectionChanged(const QVariant& selectionData)
{
    const SelectionItem* pSelectionItem = selectionData.value<mne::disp::SelectionItem*>();
    if (!pSelectionItem) {
        mne::Log::debug("AverageLayoutView::channelSelectionChanged - no SelectionItem. Returning. ");
        return;
    }
    channelSelectionManagerChanged(*pSelectionItem);
}

//=============================================================================================================

void AverageLayoutView::updateData()
{
    if (!m_pFiffInfo && (!m_pEvokedSetModel || !m_pChannelInfoModel)) {
        mne::Log::debug("AverageLayoutView::updateData - m_pEvokedSetModel or m_pChannelInfoModel are NULL. Returning. ");
        return;
    }

    // Column 2 of the evoked set model holds the averages, each a column-major channels x samples matrix
    std::vector<AvrTypeRowVectorPair> averageData;
    if (m_pEvokedSetModel) {
        averageData = m_pEvokedSetModel->data(0, 2, EvokedSetModelRoles::GetAverageData).value<std::vector<AvrTypeRowVectorPair>>();
    }

    for (AverageLayoutWidget::Channel& channel : m_pAverageLayoutWidget->channels()) {
        channel.averages.clear();
        if (!m_pEvokedSetModel) {
            continue;
        }

        // With a FiffInfo the selection carries the channel numbers, otherwise the names are looked up
        if (!m_pFiffInfo) {
            channel.number = m_pChannelInfoModel->getIndexFromMappedChName(channel.name);
        }
        const int channelNumber = channel.number;
        if (channelNumber == -1) {
            continue;
        }

        if (m_pFiffInfo) {
            channel.kind = m_pFiffInfo->chs.at(channelNumber).kind;
            channel.unit = m_pFiffInfo->chs.at(channelNumber).unit;
        } else {
            channel.kind = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 4), ChannelInfoModelRoles::GetChKind).toInt();
            channel.unit = m_pChannelInfoModel->data(m_pChannelInfoModel->index(channelNumber, 6), ChannelInfoModelRoles::GetChUnit).toInt();
        }
        channel.firstSample = -m_pEvokedSetModel->getNumPreStimSamples();
        channel.bad = m_pEvokedSetModel->getIsChannelBad(channelNumber);
        const int channelCount = m_pEvokedSetModel->rowCount();
        for (const auto& [comment, average] : averageData) {
            channel.averages.push_back({comment, average.first + channelNumber, average.second, channelCount});
        }
    }

    m_pAverageLayoutWidget->update();
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
