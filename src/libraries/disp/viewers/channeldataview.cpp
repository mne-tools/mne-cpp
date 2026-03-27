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
#include "helpers/channellabelpanel.h"
#include "helpers/channelrhiview.h"
#include "helpers/timerulerwidget.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QToolButton>
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

    // ── Render area ──────────────────────────────────────────────────────
    // Layout:
    //   renderRow (HBox)
    //     leftCol  (VBox): [rulerSpacer(28px) | labelPanel]
    //     traceCol (VBox): [ruler(28px)       | rhiView   ]
    //     rightCol (VBox): [scrollModeButton  | channelScrollBar]

    auto *renderRow = new QHBoxLayout();
    renderRow->setContentsMargins(0, 0, 0, 0);
    renderRow->setSpacing(0);

    // ── Left column: spacer aligned with ruler + label panel below ───────
    auto *leftCol = new QVBoxLayout();
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(0);

    // Invisible spacer whose height matches the time ruler (28 px fixed).
    auto *rulerSpacer = new QWidget(this);
    rulerSpacer->setFixedHeight(28);
    rulerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    leftCol->addWidget(rulerSpacer, 0);

    m_pLabelPanel = new ChannelLabelPanel(this);
    leftCol->addWidget(m_pLabelPanel, 1);

    renderRow->addLayout(leftCol, 0);

    // ── Centre column: time ruler + RHI view ─────────────────────────────
    auto *traceColumn = new QVBoxLayout();
    traceColumn->setContentsMargins(0, 0, 0, 0);
    traceColumn->setSpacing(0);

    m_pTimeRuler = new TimeRulerWidget(this);
    traceColumn->addWidget(m_pTimeRuler, 0);

    m_pRhiView = new ChannelRhiView(this);
    m_pRhiView->setModel(m_pModel.data());
    m_pRhiView->setBackgroundColor(m_bgColor);
    m_pRhiView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pRhiView->setFocusPolicy(Qt::NoFocus);
    traceColumn->addWidget(m_pRhiView, 1);

    renderRow->addLayout(traceColumn, 1);

    // ── Right column: scroll-mode toggle + channel scrollbar ─────────────
    auto *rightCol = new QVBoxLayout();
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(0);

    m_pScrollModeButton = new QToolButton(this);
    m_pScrollModeButton->setCheckable(true);
    m_pScrollModeButton->setChecked(true);  // default: ↕ = channels
    m_pScrollModeButton->setText(QStringLiteral("\u2195 Ch"));
    m_pScrollModeButton->setFixedHeight(28); // aligns with ruler
    m_pScrollModeButton->setToolTip(QStringLiteral(
        "Vertical mouse wheel:\n"
        "\u2195 Ch  \u2013 scroll through channels\n"
        "\u21c4 Time \u2013 scroll through time"));
    rightCol->addWidget(m_pScrollModeButton, 0);

    m_pChannelScrollBar = new QScrollBar(Qt::Vertical, this);
    m_pChannelScrollBar->setMinimum(0);
    m_pChannelScrollBar->setMaximum(0);
    m_pChannelScrollBar->setValue(0);
    m_pChannelScrollBar->setSingleStep(1);
    m_pChannelScrollBar->setPageStep(12);
    m_pChannelScrollBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    rightCol->addWidget(m_pChannelScrollBar, 1);

    renderRow->addLayout(rightCol, 0);

    outerLayout->addLayout(renderRow, 1);

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

    connect(m_pChannelScrollBar, &QScrollBar::valueChanged,
            this, &ChannelDataView::onChannelScrollBarMoved);

    connect(m_pRhiView, &ChannelRhiView::scrollSampleChanged,
            this, &ChannelDataView::onRhiScrollChanged);

    connect(m_pRhiView, &ChannelRhiView::channelOffsetChanged,
            this, &ChannelDataView::onChannelOffsetChanged);

    connect(m_pLabelPanel, &ChannelLabelPanel::channelScrollRequested,
            m_pRhiView, &ChannelRhiView::setFirstVisibleChannel);

    connect(m_pRhiView, &ChannelRhiView::channelOffsetChanged,
            m_pLabelPanel, &ChannelLabelPanel::setFirstVisibleChannel);

    connect(m_pScrollModeButton, &QToolButton::toggled, this, [this](bool checked) {
        // checked = ↕ channels mode; unchecked = ⟷ time mode
        m_pRhiView->setWheelScrollsChannels(checked);
        m_pScrollModeButton->setText(checked ? QStringLiteral("\u2195 Ch")
                                             : QStringLiteral("\u21c4 Time"));
    });

    connect(m_pRhiView, &ChannelRhiView::sampleClicked,
            this, &ChannelDataView::sampleClicked);

    connect(m_pRhiView, &ChannelRhiView::scrollSampleChanged,
            m_pTimeRuler, &TimeRulerWidget::setScrollSample);

    connect(m_pRhiView, &ChannelRhiView::samplesPerPixelChanged,
            m_pTimeRuler, &TimeRulerWidget::setSamplesPerPixel);

    connect(m_pModel.data(), &ChannelDataModel::dataChanged,
            this, &ChannelDataView::updateScrollBarRange);

    connect(m_pModel.data(), &ChannelDataModel::metaChanged,
            this, &ChannelDataView::updateChannelScrollBarRange);

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

    if (pInfo) {
        if (m_pTimeRuler)
            m_pTimeRuler->setSfreq(pInfo->sfreq);
        if (m_pRhiView)
            m_pRhiView->setSfreq(static_cast<float>(pInfo->sfreq));
    }

    if (m_pLabelPanel) {
        m_pLabelPanel->setModel(m_pModel.data());
        m_pLabelPanel->setVisibleChannelCount(m_pRhiView ? m_pRhiView->visibleChannelCount() : 12);
    }

    updateSamplesPerPixel();
    updateScrollBarRange();
    updateChannelScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::setData(const MatrixXd &data, int firstSample)
{
    m_pModel->setData(data, firstSample);
    if (m_pTimeRuler)
        m_pTimeRuler->setFirstFileSample(firstSample);
    if (m_pRhiView)
        m_pRhiView->setFirstFileSample(firstSample);
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

void ChannelDataView::setRemoveDC(bool dc)
{
    m_pModel->setRemoveDC(dc);
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
    float rawWindowSec  = s.value(m_sSettingsPath + "/windowSizeSeconds", 10.f).toFloat();
    m_windowSizeSeconds = qBound(0.5f, rawWindowSec, 120.f);
    // Remove stale out-of-range value so next session starts clean.
    if (rawWindowSec != m_windowSizeSeconds)
        s.remove(m_sSettingsPath + "/windowSizeSeconds");

    m_zoomFactor  = s.value(m_sSettingsPath + "/zoomFactor",        1.0).toDouble();
    m_bgColor     = s.value(m_sSettingsPath + "/backgroundColor",   QColor(250, 250, 250)).value<QColor>();
    m_signalColor = s.value(m_sSettingsPath + "/signalColor",       QColor(Qt::darkGreen)).value<QColor>();

    // Propagate loaded colors to the sub-views (they may have been created
    // with defaults in setupLayout() before loadSettings() ran).
    if (m_pRhiView)
        m_pRhiView->setBackgroundColor(m_bgColor);
    if (m_pModel)
        m_pModel->setSignalColor(m_signalColor);
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
    if (m_pTimeRuler) {
        m_pTimeRuler->setFirstFileSample(0);
        m_pTimeRuler->setScrollSample(0.f);
    }
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

    int firstSamp = m_pModel->firstSample();
    int total     = m_pModel->totalSamples();
    int visible   = m_pRhiView->visibleSampleCount();
    int minVal    = firstSamp;
    int maxVal    = qMax(firstSamp, firstSamp + total - visible);

    m_scrollBarUpdating = true;
    m_pScrollBar->setMinimum(minVal);
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

//=============================================================================================================

void ChannelDataView::onChannelScrollBarMoved(int value)
{
    if (m_channelScrollBarUpdating || !m_pRhiView)
        return;
    m_pRhiView->setFirstVisibleChannel(value);
}

//=============================================================================================================

void ChannelDataView::onChannelOffsetChanged(int firstChannel)
{
    if (!m_pChannelScrollBar)
        return;
    m_channelScrollBarUpdating = true;
    m_pChannelScrollBar->setValue(firstChannel);
    m_channelScrollBarUpdating = false;
}

//=============================================================================================================

void ChannelDataView::updateChannelScrollBarRange()
{
    if (!m_pChannelScrollBar || !m_pRhiView)
        return;

    int totalCh    = m_pModel->channelCount();
    int visibleCnt = m_pRhiView->visibleChannelCount();
    int maxVal     = qMax(0, totalCh - visibleCnt);

    m_channelScrollBarUpdating = true;
    m_pChannelScrollBar->setMaximum(maxVal);
    m_pChannelScrollBar->setPageStep(visibleCnt);
    m_channelScrollBarUpdating = false;
}
