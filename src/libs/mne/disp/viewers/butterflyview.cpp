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
#include "../plots/helpers/svgwriter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPainterPath>
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
    const QImage frame = grab().toImage();

    if (fileName.contains(".svg", Qt::CaseInsensitive)) {
        SvgWriter svgGen;
        svgGen.setFileName(fileName.toStdString());
        svgGen.setSize(this->size());
        svgGen.setViewBox(this->rect());

        QPainter painter(&svgGen);
        painter.drawImage(this->rect(), frame);
    }

    if (fileName.contains(".png", Qt::CaseInsensitive)) {
        frame.save(fileName);
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

    painter.save();
    painter.setBrush(QBrush(m_colCurrentBackgroundColor));
    painter.drawRect(QRect(-1, -1, this->width() + 2, this->height() + 2));
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_bIsInit && m_pEvokedSetModel) {
        //Draw baseline correction area
        {
            const auto [from, to] = m_pEvokedSetModel->getBaselineInfo();

            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            painter.setBrush(Qt::red);
            painter.setOpacity(0.1);

            if (m_pEvokedSetModel->getNumSamples() == 0) {
                mne::Log::debug("Unable to get data. Returning early.");
                return;
            }

            float fDx = static_cast<float>(this->width()) / (static_cast<float>(m_pEvokedSetModel->getNumSamples()));

            float fromSamp = ((from)*m_pEvokedSetModel->getSamplingFrequency()) + m_pEvokedSetModel->getNumPreStimSamples();
            float posX = fDx * (fromSamp);
            float toSamp = ((to)*m_pEvokedSetModel->getSamplingFrequency()) + m_pEvokedSetModel->getNumPreStimSamples();
            float width = fDx * (toSamp - fromSamp);

            QRect rect(posX, 0, width, this->height());

            painter.drawRect(rect);

            painter.restore();
        }

        //Stimulus bar
        if (m_pEvokedSetModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));

            float fDx = static_cast<float>(this->width()) / (static_cast<float>(m_pEvokedSetModel->getNumSamples()));
            float posX = fDx * (static_cast<float>(m_pEvokedSetModel->getNumPreStimSamples()));
            painter.drawLine(posX, 1, posX, this->height());

            painter.drawText(QPointF(posX + 5, this->rect().bottomRight().y() - 5), QString("0ms / Stimulus"));

            painter.restore();
        }

        //Vertical time spacers
        if (m_pEvokedSetModel->getNumberOfTimeSpacers() > 0) {
            painter.save();
            QColor colorTimeSpacer = Qt::black;
            colorTimeSpacer.setAlphaF(0.5);
            painter.setPen(QPen(colorTimeSpacer, 1, Qt::DashLine));

            float yStart = this->rect().topLeft().y();
            float yEnd = this->rect().bottomRight().y();

            float fDx = 1;
            if (m_pEvokedSetModel->getNumSamples() != 0) {
                fDx = static_cast<float>(this->width()) / (static_cast<float>(m_pEvokedSetModel->getNumSamples()));
            }

            float sampleCounter = m_pEvokedSetModel->getNumPreStimSamples();
            int counter = 1;
            float timeDistanceMSec = 50.0;
            float timeDistanceSamples = (timeDistanceMSec / 1000.0) * m_pEvokedSetModel->getSamplingFrequency(); //time distance corresponding to sampling frequency

            //spacers before stim
            while (sampleCounter - timeDistanceSamples > 0) {
                sampleCounter -= timeDistanceSamples;
                float x = fDx * sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x + 5, yEnd - 5), QString("-%1ms").arg(timeDistanceMSec * counter));
                counter++;
            }

            //spacers after stim
            counter = 1;
            sampleCounter = m_pEvokedSetModel->getNumPreStimSamples();
            while (sampleCounter + timeDistanceSamples < m_pEvokedSetModel->getNumSamples()) {
                sampleCounter += timeDistanceSamples;
                float x = fDx * sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x + 5, yEnd - 5), QString("%1ms").arg(timeDistanceMSec * counter));
                counter++;
            }

            painter.restore();
        }

        //Zero line
        if (m_pEvokedSetModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            painter.drawLine(0, this->height() / 2, this->width(), this->height() / 2);

            painter.restore();
        }

        painter.translate(0, this->height() / 2);

        //Actual average data
        for (qint32 r = 0; r < m_pEvokedSetModel->rowCount(); ++r) {
            if (std::ranges::contains(m_lSelectedChannels, r)) {
                qint32 kind = m_pEvokedSetModel->getKind(r);

                //Display only selected kinds
                switch (kind) {
                    case FIFFV_MEG_CH: {
                        qint32 unit = m_pEvokedSetModel->getUnit(r);
                        if (unit == FIFF_UNIT_T_M) {
                            if (m_modalityMap["GRAD"])
                                break;
                            else
                                continue;
                        } else if (unit == FIFF_UNIT_T) {
                            if (m_modalityMap["MAG"])
                                break;
                            else
                                continue;
                        }
                        continue;
                    }
                    case FIFFV_EEG_CH: {
                        if (m_modalityMap["EEG"])
                            break;
                        else
                            continue;
                    }
                    case FIFFV_EOG_CH: {
                        if (m_modalityMap["EOG"])
                            break;
                        else
                            continue;
                    }
                    case FIFFV_MISC_CH: {
                        if (m_modalityMap["MISC"])
                            break;
                        else
                            continue;
                    }
                    default:
                        continue;
                }

                painter.save();

                createPlotPath(r, painter);

                painter.restore();
            }
        }
    }
}

//=============================================================================================================

void ButterflyView::createPlotPath(qint32 row, QPainter& painter) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = m_pEvokedSetModel->getKind(row);
    float fMaxValue = mne::disp::getScalingValue(m_scaleMap, kind, m_pEvokedSetModel->getUnit(row));
    bool bIsBad = m_pEvokedSetModel->getIsChannelBad(row);

    if (bIsBad) {
        painter.setOpacity(0.20);
    } else {
        painter.setOpacity(0.75);
    }

    float fValue;
    float fScaleY = this->height() / (2 * fMaxValue);

    //restrictions for paint performance
    float fWinMaxVal = (static_cast<float>(this->height()) - 2) / 2.0f;
    //    qint32 iDownSampling = (m_pEvokedSetModel->getNumSamples() * 4 / (this->width()-2));
    //    if(iDownSampling < 1) {
    //        iDownSampling = 1;
    //    }

    QPointF qSamplePosition;

    float fDx = static_cast<float>(this->width() - 2) / (static_cast<float>(m_pEvokedSetModel->getNumSamples()) - 1.0f); //((float)option.rect.width()) / m_pEvokedSetModel->getMaxSamples();

    const std::vector<mne::disp::AvrTypeRowVector> rowVec = m_pEvokedSetModel->data(row, 1).value<std::vector<mne::disp::AvrTypeRowVector>>();

    //Do for all average types
    for (int j = 0; j < std::ssize(rowVec); ++j) {
        const std::string& sAvrComment = rowVec.at(j).first;

        // Select color for each average
        if (m_pEvokedSetModel->isFreezed()) {
            QColor freezeColor = detail::valueOr(*m_qMapAverageColor, sAvrComment);
            freezeColor.setAlphaF(0.5);
            painter.setPen(QPen(freezeColor, 1));
        } else {
            painter.setPen(QPen(detail::valueOr(*m_qMapAverageColor, sAvrComment)));
        }

        if (detail::valueOr(*m_qMapAverageActivation, sAvrComment)) {
            //Calculate downsampling factor of averaged data in respect to the items width
            int dsFactor;
            rowVec.at(j).second.cols() / this->width() < 1 ? dsFactor = 1 : dsFactor = rowVec.at(j).second.cols() / this->width();
            if (dsFactor == 0) {
                dsFactor = 1;
            }

            QPainterPath path(QPointF(1, 0));
            float y_base = path.currentPosition().y();

            //Move to initial starting point
            if (rowVec.at(j).second.cols() > 0) {
                float val = rowVec.at(j).second[0];
                fValue = (val /*-rowVec.at(j)[m_pEvokedSetModel->getNumPreStimSamples()-1]*/) * fScaleY; //ToDo -> -2 PreStim is one too short

                float newY = y_base + fValue;

                qSamplePosition.setY(-newY);
                qSamplePosition.setX(path.currentPosition().x());

                path.moveTo(qSamplePosition);
            }

            //create lines from one to the next sample
            qint32 i;
            for (i = 1; i < rowVec.at(j).second.cols() && path.elementCount() <= this->width(); i += dsFactor) {
                float val = /*rowVec.at(j)[m_pEvokedSetModel->getNumPreStimSamples()-1] - */ rowVec.at(j).second[i]; //remove first sample data[0] as offset
                fValue = val * fScaleY;

                //Cut plotting if out of widget area
                fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal
                                                                                 : fValue;

                float newY = y_base + fValue;

                qSamplePosition.setY(-newY);

                qSamplePosition.setX(1.0 + fDx * i);

                path.lineTo(qSamplePosition);
            }

            //    //create lines from one to the next sample for last path
            //    qint32 sample_offset = m_pEvokedSetModel->numVLines() + 1;
            //    qSamplePosition.setX(qSamplePosition.x() + fDx*sample_offset);
            //    lastPath.moveTo(qSamplePosition);

            //    for(i += sample_offset; i < lastData.size(); ++i) {
            //        float val = lastData[i] - lastData[0]; //remove first sample lastData[0] as offset
            //        fValue = val*fScaleY;

            //        float newY = y_base+fValue;

            //        qSamplePosition.setY(newY);
            //        qSamplePosition.setX(lastPath.currentPosition().x()+fDx);

            //        lastPath.lineTo(qSamplePosition);
            //    }

            painter.drawPath(path);
        }
    }
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
