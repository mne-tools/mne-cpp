//=============================================================================================================
/**
 * @file     viewporttimestrip.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    ViewportTimeStrip class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewporttimestrip.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ViewportTimeStrip::ViewportTimeStrip(int viewportIdx, QWidget *parent)
    : QWidget(parent)
    , m_viewportIdx(viewportIdx)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    m_stepBackBtn = new QPushButton(QStringLiteral("\u25C0"));
    m_stepBackBtn->setFixedWidth(28);
    m_stepBackBtn->setToolTip("Step backward");

    m_playBtn = new QPushButton(QStringLiteral("\u25B6"));
    m_playBtn->setFixedWidth(28);
    m_playBtn->setToolTip("Play / Pause");

    m_stepFwdBtn = new QPushButton(QStringLiteral("\u25B6\u25B6"));
    m_stepFwdBtn->setFixedWidth(28);
    m_stepFwdBtn->setToolTip("Step forward");

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 0);
    m_slider->setEnabled(false);

    m_timeLabel = new QLabel("Time: 0.000 s");
    m_timeLabel->setMinimumWidth(100);

    layout->addWidget(m_stepBackBtn);
    layout->addWidget(m_playBtn);
    layout->addWidget(m_stepFwdBtn);
    layout->addWidget(m_slider, 1);
    layout->addWidget(m_timeLabel);

    setMaximumHeight(32);

    // Connections
    connect(m_stepBackBtn, &QPushButton::clicked, [this]() {
        emit stepBackward(m_viewportIdx);
    });

    connect(m_stepFwdBtn, &QPushButton::clicked, [this]() {
        emit stepForward(m_viewportIdx);
    });

    connect(m_playBtn, &QPushButton::clicked, [this]() {
        m_playing = !m_playing;
        m_playBtn->setText(m_playing ? QStringLiteral("\u23F8") : QStringLiteral("\u25B6"));
        emit playToggled(m_viewportIdx, m_playing);
    });

    connect(m_slider, &QSlider::valueChanged, [this](int value) {
        emit sliderValueChanged(m_viewportIdx, value);
    });
}

//=============================================================================================================

void ViewportTimeStrip::setRange(int maxIndex)
{
    m_slider->setRange(0, maxIndex);
    m_slider->setEnabled(maxIndex > 0);
}

//=============================================================================================================

void ViewportTimeStrip::setTimePoint(int index, float time)
{
    m_slider->blockSignals(true);
    m_slider->setValue(index);
    m_slider->blockSignals(false);
    m_timeLabel->setText(QString("Time: %1 s").arg(static_cast<double>(time), 0, 'f', 3));
}

//=============================================================================================================

void ViewportTimeStrip::setControlsEnabled(bool enabled)
{
    m_stepBackBtn->setEnabled(enabled);
    m_playBtn->setEnabled(enabled);
    m_stepFwdBtn->setEnabled(enabled);
    m_slider->setEnabled(enabled && m_slider->maximum() > 0);
}

//=============================================================================================================

int ViewportTimeStrip::currentValue() const
{
    return m_slider->value();
}

//=============================================================================================================

void ViewportTimeStrip::stopPlayback()
{
    m_playing = false;
    m_playBtn->setText(QStringLiteral("\u25B6"));
}
