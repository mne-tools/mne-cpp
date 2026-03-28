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
#include <QPainter>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE HELPER WIDGET — StimEventStrip
// A thin horizontal strip placed above the time ruler that draws coloured
// event-type chips at the sample positions of loaded stimulus events.
//=============================================================================================================

class StimEventStrip : public QWidget
{
public:
    static constexpr int kHeight = 22; // px — fixed height of the strip

    explicit StimEventStrip(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(kHeight);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setEvents(const QVector<ChannelRhiView::EventMarker> &ev)
    {
        m_events = ev;
        update();
    }

    void setScrollSample(float s)
    {
        m_scrollSample = s;
        update();
    }

    void setSamplesPerPixel(float spp)
    {
        if (spp > 0.f) m_spp = spp;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        // Background: slightly distinct from both the channel area and the ruler
        p.fillRect(rect(), QColor(232, 232, 240));

        // Bottom border line
        p.setPen(QPen(QColor(190, 190, 205), 1));
        p.drawLine(0, kHeight - 1, width(), kHeight - 1);

        if (m_events.isEmpty() || m_spp <= 0.f)
            return;

        QFont f;
        f.setPixelSize(9);
        f.setBold(true);
        p.setFont(f);

        // Track occupied x-ranges to avoid complete chip overlap; shift a duplicate
        // a few pixels to the right so stacked events remain readable.
        QVector<int> usedX;

        for (const ChannelRhiView::EventMarker &ev : m_events) {
            float xF = (static_cast<float>(ev.sample) - m_scrollSample) / m_spp;
            if (xF < -2.f || xF > width() + 2.f)
                continue;
            int ix = static_cast<int>(xF);

            // Tick mark at the bottom of the strip (will align with the ruler below)
            QColor tickCol = ev.color;
            tickCol.setAlpha(200);
            p.setPen(QPen(tickCol, 1));
            p.drawLine(ix, kHeight - 4, ix, kHeight - 1);

            // Colour chip: centred on ix, shifted right if already occupied
            constexpr int kChipW = 26, kChipH = 14;
            int chipX = qBound(0, ix - kChipW / 2, width() - kChipW);
            // nudge right if overlapping a previous chip
            while (usedX.contains(chipX / 4)) chipX = qMin(chipX + 4, width() - kChipW);
            usedX.append(chipX / 4);

            const int chipY = (kHeight - kChipH) / 2 - 1;
            QRectF chip(chipX, chipY, kChipW, kChipH);
            QColor col = ev.color;
            col.setAlpha(220);
            p.fillRect(chip, col);
            p.setPen(Qt::white);
            QString lbl = ev.label.isEmpty() ? QString::number(ev.type) : ev.label;
            p.drawText(chip, Qt::AlignCenter, lbl);
        }
    }

private:
    QVector<ChannelRhiView::EventMarker> m_events;
    float m_scrollSample = 0.f;
    float m_spp          = 1.f;
};

//=============================================================================================================
// PRIVATE HELPER WIDGET — TimeHeaderLabel
// Left-column header that labels the time ruler row with "Time" and the unit.
//=============================================================================================================

class TimeHeaderLabel : public QWidget
{
public:
    explicit TimeHeaderLabel(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(28); // matches TimeRulerWidget::sizeHint height
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.fillRect(rect(), QColor(242, 242, 242));

        // Right-side separator
        p.setPen(QPen(QColor(200, 200, 200), 1));
        p.drawLine(width() - 1, 0, width() - 1, height());

        // "Time" bold
        QFont boldF = font();
        boldF.setPointSizeF(8.0);
        boldF.setBold(true);
        p.setFont(boldF);
        p.setPen(QColor(60, 60, 70));
        p.drawText(QRect(0, 1, width(), height() / 2), Qt::AlignCenter, QStringLiteral("Time"));

        // "mm:ss·ms" sub-label
        QFont subF = font();
        subF.setPointSizeF(6.5);
        p.setFont(subF);
        p.setPen(QColor(130, 130, 145));
        p.drawText(QRect(0, height() / 2, width(), height() / 2), Qt::AlignCenter,
                   QStringLiteral("mm:ss\u00B7ms"));  // · = U+00B7
    }
};

//=============================================================================================================
// PRIVATE HELPER WIDGET — StimHeaderLabel
// Left-column header that labels the stimulus strip row.
//=============================================================================================================

class StimHeaderLabel : public QWidget
{
public:
    explicit StimHeaderLabel(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(StimEventStrip::kHeight);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.fillRect(rect(), QColor(236, 236, 244));

        // Right-side separator
        p.setPen(QPen(QColor(200, 200, 200), 1));
        p.drawLine(width() - 1, 0, width() - 1, height());

        // Bottom border to match the strip
        p.setPen(QPen(QColor(190, 190, 205), 1));
        p.drawLine(0, height() - 1, width() - 1, height() - 1);

        // "Stim" label
        QFont f = font();
        f.setPointSizeF(8.0);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(80, 80, 95));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Stim"));
    }
};

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
    //     leftCol  (VBox): [stimHdr(22px) | timeHdr(28px) | labelPanel   ]
    //     traceCol (VBox): [stimStrip(22px) | ruler(28px) | rhiView(flex)]
    //     rightCol (VBox): [spacer(22px) | scrollModeBtn(28px) | chanSB   ]

    auto *renderRow = new QHBoxLayout();
    renderRow->setContentsMargins(0, 0, 0, 0);
    renderRow->setSpacing(0);

    // ── Left column ───────────────────────────────────────────────────────
    auto *leftCol = new QVBoxLayout();
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(0);

    // Header aligned with stimulus strip
    auto *stimHdr = new StimHeaderLabel(this);
    leftCol->addWidget(stimHdr, 0);

    // Header aligned with time ruler — shows "Time" + unit sub-label
    auto *timeHdr = new TimeHeaderLabel(this);
    leftCol->addWidget(timeHdr, 0);

    m_pLabelPanel = new ChannelLabelPanel(this);
    leftCol->addWidget(m_pLabelPanel, 1);

    renderRow->addLayout(leftCol, 0);

    // ── Centre column: stim strip + time ruler + RHI view ────────────────
    auto *traceColumn = new QVBoxLayout();
    traceColumn->setContentsMargins(0, 0, 0, 0);
    traceColumn->setSpacing(0);

    m_pStimStrip = new StimEventStrip(this);
    traceColumn->addWidget(m_pStimStrip, 0);

    m_pTimeRuler = new TimeRulerWidget(this);
    traceColumn->addWidget(m_pTimeRuler, 0);

    m_pRhiView = new ChannelRhiView(this);
    m_pRhiView->setModel(m_pModel.data());
    m_pRhiView->setBackgroundColor(m_bgColor);
    m_pRhiView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pRhiView->setFocusPolicy(Qt::NoFocus);
    traceColumn->addWidget(m_pRhiView, 1);

    renderRow->addLayout(traceColumn, 1);

    // ── Right column: spacer + scroll-mode toggle + channel scrollbar ─────
    auto *rightCol = new QVBoxLayout();
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(0);

    // Blank spacer matching stim strip height
    auto *stimRightSpacer = new QWidget(this);
    stimRightSpacer->setFixedHeight(StimEventStrip::kHeight);
    stimRightSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rightCol->addWidget(stimRightSpacer, 0);

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

    // Keep stim strip in sync with scroll / zoom
    connect(m_pRhiView, &ChannelRhiView::scrollSampleChanged,
            this, [this](float s) {
                static_cast<StimEventStrip*>(m_pStimStrip)->setScrollSample(s);
            });
    connect(m_pRhiView, &ChannelRhiView::samplesPerPixelChanged,
            this, [this](float spp) {
                static_cast<StimEventStrip*>(m_pStimStrip)->setSamplesPerPixel(spp);
            });

    connect(m_pModel.data(), &ChannelDataModel::dataChanged,
            this, &ChannelDataView::updateScrollBarRange);

    connect(m_pModel.data(), &ChannelDataModel::metaChanged,
            this, &ChannelDataView::updateChannelScrollBarRange);

    // Keep label panel in sync with model changes so status pills update immediately.
    connect(m_pModel.data(), &ChannelDataModel::metaChanged,
            m_pLabelPanel, QOverload<>::of(&QWidget::update));
    connect(m_pModel.data(), &ChannelDataModel::dataChanged,
            m_pLabelPanel, QOverload<>::of(&QWidget::update));

    // Feed current visible sample window to the label panel for the RMS level bar.
    connect(m_pRhiView, &ChannelRhiView::scrollSampleChanged,
            this, [this](float sample) {
                if (m_pLabelPanel && m_pRhiView) {
                    m_pLabelPanel->setVisibleSampleRange(
                        static_cast<int>(sample),
                        static_cast<int>(sample) + m_pRhiView->visibleSampleCount());
                }
            });

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

void ChannelDataView::setFileBounds(int first, int last)
{
    m_firstFileSample = first;
    m_lastFileSample  = last;
    if (m_pRhiView) {
        m_pRhiView->setFirstFileSample(first);
        m_pRhiView->setLastFileSample(last);
    }
    if (m_pTimeRuler)
        m_pTimeRuler->setFirstFileSample(first);
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
    if (m_pRhiView)
        m_pRhiView->setHideBadChannels(hide);
    if (m_pLabelPanel)
        m_pLabelPanel->setHideBadChannels(hide);
}

//=============================================================================================================

bool ChannelDataView::badChannelsHidden() const
{
    return m_hideBadChannels;
}

//=============================================================================================================

void ChannelDataView::setChannelFilter(const QStringList &names)
{
    QVector<int> indices;
    if (!names.isEmpty() && m_pModel) {
        int total = m_pModel->channelCount();
        for (int i = 0; i < total; ++i) {
            if (names.contains(m_pModel->channelInfo(i).name))
                indices.append(i);
        }
    }

    if (m_pRhiView)
        m_pRhiView->setChannelIndices(indices);
    if (m_pLabelPanel)
        m_pLabelPanel->setChannelIndices(indices);

    // Reset scroll to start of filtered set and update scrollbar range
    updateChannelScrollBarRange();
}

//=============================================================================================================

void ChannelDataView::setRemoveDC(bool dc)
{
    m_pModel->setRemoveDC(dc);
}

//=============================================================================================================

void ChannelDataView::setEvents(const QVector<ChannelRhiView::EventMarker> &events)
{
    if (m_pRhiView)
        m_pRhiView->setEvents(events);
    if (m_pStimStrip)
        static_cast<StimEventStrip*>(m_pStimStrip)->setEvents(events);
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
    m_pModel->clearData();
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

    int visible = m_pRhiView->visibleSampleCount();
    int minVal, maxVal;

    if (m_firstFileSample >= 0 && m_lastFileSample >= 0) {
        // File bounds known: allow scrolling across the whole file
        minVal = m_firstFileSample;
        maxVal = qMax(m_firstFileSample, m_lastFileSample - visible + 1);
    } else {
        // Fall back to ring-buffer bounds
        int firstSamp = m_pModel->firstSample();
        int total     = m_pModel->totalSamples();
        minVal = firstSamp;
        maxVal = qMax(firstSamp, firstSamp + total - visible);
    }

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

    // Use the view's logical channel count (respects active filter)
    int totalCh    = m_pRhiView ? m_pRhiView->totalLogicalChannels()
                                : m_pModel->channelCount();
    int visibleCnt = m_pRhiView->visibleChannelCount();
    int maxVal     = qMax(0, totalCh - visibleCnt);

    m_channelScrollBarUpdating = true;
    m_pChannelScrollBar->setMaximum(maxVal);
    m_pChannelScrollBar->setPageStep(visibleCnt);
    m_channelScrollBarUpdating = false;
}
