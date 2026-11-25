//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     butterflyview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Wayne F. Mead <isk@imsorrykun.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ButterflyView butterfly evoked viewer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflyview.h"
#include "../detail/map_value.h"
#include <mne/base/log.h>

#include "scalingview.h"
#include "helpers/evokedsetmodel.h"
#include "helpers/channelinfomodel.h"
#include "../plots/helpers/scenepainter.h"
#include <mne/scene/butterfly_plot.h>
#include <mne/scene/svg_renderer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflyView::ButterflyView(const QString& sSettingsPath,
                             QWidget* parent,
                             Qt::WindowFlags f)
: QWidget(parent, f)
, m_bShowMAG(true)
, m_bShowGRAD(true)
, m_bShowEEG(true)
, m_bShowEOG(true)
, m_bShowMISC(true)
, m_bIsInit(false)
, m_sSettingsPath(sSettingsPath)
, m_colCurrentBackgroundColor(Qt::white)
, m_pEvokedSetModel(nullptr)
, m_qMapAverageActivation(std::make_shared<std::map<std::string, bool>>())
, m_qMapAverageColor(std::make_shared<std::map<std::string, QColor>>())
{
    m_sSettingsPath = sSettingsPath;

    loadSettings();
}

//=============================================================================================================

ButterflyView::~ButterflyView()
{
    saveSettings();
}

//=============================================================================================================

void ButterflyView::updateViewport()
{
    update();
}

//=============================================================================================================

void ButterflyView::setEvokedSetModel(std::shared_ptr<EvokedSetModel> model)
{
    m_pEvokedSetModel = std::move(model);

    connect(m_pEvokedSetModel.get(), &EvokedSetModel::dataChanged,
            this, &ButterflyView::dataUpdate, Qt::UniqueConnection);
}

//=============================================================================================================

void ButterflyView::dataUpdate()
{
    if (!m_bIsInit && m_pEvokedSetModel->isInit()) {
        m_bIsInit = true;
    }

    setAverageActivation(m_qMapAverageActivation);

    update();
}

//=============================================================================================================

std::map<std::string, bool> ButterflyView::getModalityMap()
{
    return m_modalityMap;
}

//=============================================================================================================

void ButterflyView::setModalityMap(const std::map<std::string, bool>& modalityMap)
{
    m_modalityMap = modalityMap;
    update();
}

//=============================================================================================================

void ButterflyView::setScaleMap(const std::map<int, float>& scaleMap)
{
    m_scaleMap = scaleMap;
    update();
}

//=============================================================================================================

void ButterflyView::setSelectedChannels(const std::vector<int>& selectedChannels)
{
    m_lSelectedChannels = selectedChannels;
    update();
}

//=============================================================================================================

void ButterflyView::updateView()
{
    update();
}

//=============================================================================================================

void ButterflyView::setBackgroundColor(const QColor& backgroundColor)
{
    m_colCurrentBackgroundColor = backgroundColor;
    update();
}

//=============================================================================================================

const QColor& ButterflyView::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}

//=============================================================================================================

void ButterflyView::takeScreenshot(const QString& fileName)
{
    if (fileName.contains(".svg", Qt::CaseInsensitive)) {
        if (const auto written = scene::write_svg(figure(), fileName.toStdU16String()); !written) {
            mne::Log::warning("{}", written.error().what());
        }
    }

    if (fileName.contains(".png", Qt::CaseInsensitive)) {
        grab().toImage().save(fileName);
    }
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, QColor>> ButterflyView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, bool>> ButterflyView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void ButterflyView::setAverageColor(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
    update();
}

//=============================================================================================================

void ButterflyView::setSingleAverageColor(const QColor& avgColor)
{
    for (auto& [key, color] : *m_qMapAverageColor)
        color = avgColor;
}

//=============================================================================================================

void ButterflyView::setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    update();
}

//=============================================================================================================

void ButterflyView::setChannelInfoModel(std::shared_ptr<ChannelInfoModel>& pChannelInfoModel)
{
    m_pChannelInfoModel = pChannelInfoModel;
}

//=============================================================================================================

void ButterflyView::showSelectedChannelsOnly(const std::vector<std::string>& selectedChannels)
{
    if (!m_pChannelInfoModel) {
        mne::Log::debug("ButterflyView::showSelectedChannelsOnly - m_pChannelInfoModel is NULL. Returning. ");
        return;
    }

    std::vector<int> selectedChannelsIndexes;
    selectedChannelsIndexes.reserve(selectedChannels.size());

    for (const std::string& channel : selectedChannels)
        selectedChannelsIndexes.push_back(m_pChannelInfoModel->getIndexFromOrigChName(channel));

    setSelectedChannels(selectedChannelsIndexes);
}

//=============================================================================================================

void ButterflyView::showSelectedChannels(const std::vector<int>& selectedChannelsIndexes)
{
    setSelectedChannels(selectedChannelsIndexes);
}

//=============================================================================================================

void ButterflyView::showAllChannels()
{
    if (m_pEvokedSetModel) {
        std::vector<int> lAllChannels;
        for (int i = 0; i < m_pEvokedSetModel->rowCount(); i++) {
            lAllChannels.push_back(i);
        }
        setSelectedChannels(lAllChannels);
    }
}

//=============================================================================================================

void ButterflyView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void ButterflyView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void ButterflyView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    paintScene(painter, figure(), font());
}

//=============================================================================================================

mne::scene::DisplayList ButterflyView::figure() const
{
    scene::ButterflyPlot plot;
    plot.set_background(toSceneColor(m_colCurrentBackgroundColor));
    if (m_bIsInit && m_pEvokedSetModel) {
        if (m_pEvokedSetModel->getNumSamples() == 0) {
            mne::Log::debug("Unable to get data. Returning early.");
        }
        const auto [from, to] = m_pEvokedSetModel->getBaselineInfo();
        plot.set_timing(m_pEvokedSetModel->getNumSamples(), m_pEvokedSetModel->getNumPreStimSamples(), m_pEvokedSetModel->getSamplingFrequency());
        plot.set_baseline(from, to);
        plot.set_time_spacers(m_pEvokedSetModel->getNumberOfTimeSpacers() > 0);
        plot.set_frozen(m_pEvokedSetModel->isFreezed());

        // The selected channels of the shown modalities
        const auto shown = [this](qint32 row) {
            switch (m_pEvokedSetModel->getKind(row)) {
                case FIFFV_MEG_CH: {
                    const qint32 unit = m_pEvokedSetModel->getUnit(row);
                    return (unit == FIFF_UNIT_T_M && detail::valueOr(m_modalityMap, std::string("GRAD"))) || (unit == FIFF_UNIT_T && detail::valueOr(m_modalityMap, std::string("MAG")));
                }
                case FIFFV_EEG_CH:
                    return detail::valueOr(m_modalityMap, std::string("EEG"));
                case FIFFV_EOG_CH:
                    return detail::valueOr(m_modalityMap, std::string("EOG"));
                case FIFFV_MISC_CH:
                    return detail::valueOr(m_modalityMap, std::string("MISC"));
                default:
                    return false;
            }
        };
        std::vector<scene::ButterflyPlot::Channel> channels;
        for (qint32 r = 0; r < m_pEvokedSetModel->rowCount(); ++r) {
            if (!std::ranges::contains(m_lSelectedChannels, r) || !shown(r)) {
                continue;
            }
            scene::ButterflyPlot::Channel channel;
            const qint32 kind = m_pEvokedSetModel->getKind(r);
            channel.max_value = mne::disp::getScalingValue(m_scaleMap, kind, m_pEvokedSetModel->getUnit(r));
            channel.bad = m_pEvokedSetModel->getIsChannelBad(r);
            for (const mne::disp::AvrTypeRowVector& average : m_pEvokedSetModel->data(r, 1).value<std::vector<mne::disp::AvrTypeRowVector>>()) {
                if (detail::valueOr(*m_qMapAverageActivation, average.first)) {
                    channel.traces.push_back({std::vector<double>(average.second.data(), average.second.data() + average.second.size()),
                                              toSceneColor(detail::valueOr(*m_qMapAverageColor, average.first))});
                }
            }
            channels.push_back(std::move(channel));
        }
        plot.set_channels(std::move(channels));
    }
    return plot.render(width(), height(), toSceneFont(font()));
}

//=============================================================================================================

std::shared_ptr<EvokedSetModel> ButterflyView::getEvokedSetModel()
{
    return m_pEvokedSetModel;
}

//=============================================================================================================

void ButterflyView::clearView()
{
    setEvokedSetModel(nullptr);
}
