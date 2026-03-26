//=============================================================================================================
/**
 * @file     channeldataview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the ChannelDataView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldataview.h"
#include "helpers/channeldatamodel.h"
#include "helpers/channelrhiview.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QLabel>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataView::ChannelDataView(const QString &sSettingsPath,
                                 QWidget *parent,
                                 Qt::WindowFlags f)
    : AbstractView(parent, f)
    , m_sSettingsPath(sSettingsPath)
    , m_pModel(new ChannelDataModel(this))
{
    setupLayout();
    loadSettings();
}

//=============================================================================================================

ChannelDataView::~ChannelDataView()
{
    saveSettings();
}

//=============================================================================================================

void ChannelDataView::setupLayout()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── Render surface ───────────────────────────────────────────────────
    m_pRhiView = new ChannelRhiView(this);
    m_pRhiView->setModel(m_pModel.data());
    m_pRhiView->setBackgroundColor(m_bgColor);
    m_pRhiView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pRhiView->setFocusPolicy(Qt::NoFocus);

    outerLayout->addWidget(m_pRhiView, 1);

    // ── Horizontal scroll bar ────────────────────────────────────────────
    m_pScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_pScrollBar->setMinimum(0);
    m_pScrollBar->setMaximum(0);
    m_pScrollBar->setValue(0);
    m_pScrollBar->setSingleStep(1);
    m_pScrollBar->setPageStep(100);
    m_pScrollBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outerLayout->addWidget(m_pScrollBar, 0);

    // ── Connections ──────────────────────────────────────────────────────
    connect(m_pScrollBar, &QScrollBar::valueChanged,
            this, &ChannelDataView::onScrollBarMoved);

    connect(m_pRhiView, &ChannelRhiView::scrollSampleChanged,
            this, &ChannelDataView::onRhiScrollChanged);

    connect(m_pRhiView, &ChannelRhiView::sampleClicked,
            this, &ChannelDataView::sampleClicked);

    connect(m_pModel.data(), &ChannelDataModel::dataChanged,
            this, &ChannelDataView::updateScrollBarRange);

    setFocusProxy(m_pRhiView);
    setFocusPolicy(Qt::StrongFocus);
}

//=============================================================================================================

void ChannelDataView::init(QSharedPointer<FiffInfo> pInfo)
{
    m_pFiffInfo = pInfo;
    m_pModel->init(pInfo);

    // Re-apply scale map and colour after re-init
    if (!m_scaleMap.isEmpty())
        m_pModel->setScaleMap(m_scaleMap);
    m_pModel->setSignalColor(m_signalColor);

    updateSamplesPerPixel();
    updateScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::setData(const MatrixXd &data, int firstSample)
{
    m_pModel->setData(data, firstSample);
    updateScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::addData(const MatrixXd &data)
{
    m_pModel->appendData(data);
    updateScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::scrollToSample(int sample, bool animate)
{
    m_pRhiView->scrollTo(static_cast<float>(sample), animate ? 200 : 0);
}

//=============================================================================================================

void ChannelDataView::setWindowSize(float seconds)
{
    m_windowSizeSeconds = qMax(seconds, 0.01f);
    updateSamplesPerPixel();
    saveSettings();
}

//=============================================================================================================

float ChannelDataView::windowSize() const
{
    return m_windowSizeSeconds;
}

//=============================================================================================================

void ChannelDataView::setZoom(double factor)
{
    m_zoomFactor = qMax(factor, 0.001);
    updateSamplesPerPixel();
    saveSettings();
}

//=============================================================================================================

double ChannelDataView::zoom() const
{
    return m_zoomFactor;
}

//=============================================================================================================

void ChannelDataView::setBackgroundColor(const QColor &color)
{
    m_bgColor = color;
    if (m_pRhiView)
        m_pRhiView->setBackgroundColor(color);
    saveSettings();
}

//=============================================================================================================

QColor ChannelDataView::backgroundColor() const
{
    return m_bgColor;
}

//=============================================================================================================

void ChannelDataView::setSignalColor(const QColor &color)
{
    m_signalColor = color;
    m_pModel->setSignalColor(color);
    saveSettings();
}

//=============================================================================================================

QColor ChannelDataView::signalColor() const
{
    return m_signalColor;
}

//=============================================================================================================

void ChannelDataView::setScalingMap(const QMap<qint32, float> &scaleMap)
{
    m_scaleMap = scaleMap;
    m_pModel->setScaleMap(scaleMap);
}

//=============================================================================================================

QMap<qint32, float> ChannelDataView::scalingMap() const
{
    return m_scaleMap;
}

//=============================================================================================================

void ChannelDataView::hideBadChannels(bool hide)
{
    m_hideBadChannels = hide;
    // Future: collapse zero-height lanes for bad channels in the renderer.
}

//=============================================================================================================

bool ChannelDataView::badChannelsHidden() const
{
    return m_hideBadChannels;
}

//=============================================================================================================

int ChannelDataView::firstVisibleSample() const
{
    return m_pRhiView ? m_pRhiView->visibleFirstSample() : 0;
}

//=============================================================================================================

void ChannelDataView::saveSettings()
{
    if (m_sSettingsPath.isEmpty())
        return;
    QSettings s;
    s.setValue(m_sSettingsPath + "/windowSizeSeconds", m_windowSizeSeconds);
    s.setValue(m_sSettingsPath + "/zoomFactor",        m_zoomFactor);
    s.setValue(m_sSettingsPath + "/backgroundColor",   m_bgColor);
    s.setValue(m_sSettingsPath + "/signalColor",       m_signalColor);
}

//=============================================================================================================

void ChannelDataView::loadSettings()
{
    if (m_sSettingsPath.isEmpty())
        return;
    QSettings s;
    m_windowSizeSeconds = s.value(m_sSettingsPath + "/windowSizeSeconds", 10.f).toFloat();
    m_zoomFactor        = s.value(m_sSettingsPath + "/zoomFactor",        1.0).toDouble();
    m_bgColor           = s.value(m_sSettingsPath + "/backgroundColor",   QColor(30, 30, 30)).value<QColor>();
    m_signalColor       = s.value(m_sSettingsPath + "/signalColor",       QColor(Qt::darkGreen)).value<QColor>();
}

//=============================================================================================================

void ChannelDataView::clearView()
{
    m_pModel->setData(MatrixXd(), 0);
    if (m_pScrollBar) {
        m_pScrollBar->setMaximum(0);
        m_pScrollBar->setValue(0);
    }
    if (m_pRhiView)
        m_pRhiView->setScrollSample(0.f);
}

//=============================================================================================================

void ChannelDataView::updateGuiMode(GuiMode /*mode*/) {}

void ChannelDataView::updateProcessingMode(ProcessingMode /*mode*/) {}

//=============================================================================================================

void ChannelDataView::keyPressEvent(QKeyEvent *event)
{
    if (!m_pRhiView) {
        QWidget::keyPressEvent(event);
        return;
    }

    float step = m_pRhiView->visibleSampleCount() * 0.1f;
    float page = m_pRhiView->visibleSampleCount() * 0.9f;

    switch (event->key()) {
    case Qt::Key_Left:
        m_pRhiView->scrollTo(m_pRhiView->scrollSample() - step, 150);
        break;
    case Qt::Key_Right:
        m_pRhiView->scrollTo(m_pRhiView->scrollSample() + step, 150);
        break;
    case Qt::Key_PageUp:
        m_pRhiView->scrollTo(m_pRhiView->scrollSample() - page, 200);
        break;
    case Qt::Key_PageDown:
        m_pRhiView->scrollTo(m_pRhiView->scrollSample() + page, 200);
        break;
    case Qt::Key_Home:
        m_pRhiView->scrollTo(static_cast<float>(m_pModel->firstSample()), 300);
        break;
    case Qt::Key_End:
        {
            float lastStart = static_cast<float>(
                m_pModel->firstSample() + m_pModel->totalSamples()
                - m_pRhiView->visibleSampleCount());
            m_pRhiView->scrollTo(qMax(lastStart, 0.f), 300);
        }
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        m_pRhiView->zoomTo(m_pRhiView->samplesPerPixel() * 0.75f, 200);
        break;
    case Qt::Key_Minus:
        m_pRhiView->zoomTo(m_pRhiView->samplesPerPixel() * 1.33f, 200);
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

//=============================================================================================================

void ChannelDataView::resizeEvent(QResizeEvent *event)
{
    AbstractView::resizeEvent(event);
    updateSamplesPerPixel();
    updateScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::onScrollBarMoved(int value)
{
    if (m_scrollBarUpdating || !m_pRhiView)
        return;
    m_pRhiView->setScrollSample(static_cast<float>(value));
}

//=============================================================================================================

void ChannelDataView::onRhiScrollChanged(float sample)
{
    emit scrollPositionChanged(static_cast<int>(sample));

    if (!m_pScrollBar)
        return;

    m_scrollBarUpdating = true;
    m_pScrollBar->setValue(static_cast<int>(sample));
    m_scrollBarUpdating = false;
}

//=============================================================================================================

void ChannelDataView::updateScrollBarRange()
{
    if (!m_pScrollBar || !m_pRhiView)
        return;

    int total   = m_pModel->totalSamples();
    int visible = m_pRhiView->visibleSampleCount();
    int maxVal  = qMax(0, total - visible);

    m_scrollBarUpdating = true;
    m_pScrollBar->setMaximum(maxVal);
    m_pScrollBar->setPageStep(visible);
    m_scrollBarUpdating = false;
}

//=============================================================================================================

void ChannelDataView::updateSamplesPerPixel()
{
    if (!m_pRhiView || !m_pFiffInfo)
        return;

    float sfreq   = static_cast<float>(m_pFiffInfo->sfreq);
    int   viewPx  = m_pRhiView->width();
    if (viewPx <= 0)
        return;

    // samples per pixel = (window_duration × sample_rate) / viewport_width / zoom
    float spp = (m_windowSizeSeconds * sfreq) / viewPx / static_cast<float>(m_zoomFactor);
    m_pRhiView->setSamplesPerPixel(qMax(spp, 1e-4f));
    updateScrollBarRange();
}
