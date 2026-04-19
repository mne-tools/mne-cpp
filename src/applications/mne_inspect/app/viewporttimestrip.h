//=============================================================================================================
/**
 * @file     viewporttimestrip.h
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
 * @brief    ViewportTimeStrip class declaration — per-viewport playback control strip.
 *
 */

#ifndef VIEWPORTTIMESTRIP_H
#define VIEWPORTTIMESTRIP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QElapsedTimer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QPushButton;
class QSlider;
class QLabel;

//=============================================================================================================
/**
 * A thin control strip for per-viewport timeline control.
 *
 * Layout: [ << ] [ Play/Pause ] [ >> ]  ────●──────  Time: 0.000 s
 *
 * @brief Per-viewport playback control strip.
 */
class ViewportTimeStrip : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] viewportIdx  Index of the viewport this strip controls.
     * @param[in] parent       Parent widget.
     */
    explicit ViewportTimeStrip(int viewportIdx, QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Set the range of the time slider.
     *
     * @param[in] maxIndex  Maximum time index.
     */
    void setRange(int maxIndex);

    //=========================================================================================================
    /**
     * Set the current time index (updates slider and label).
     *
     * @param[in] index  Time index.
     * @param[in] time   Time in seconds.
     */
    void setTimePoint(int index, float time);

    //=========================================================================================================
    /**
     * Enable or disable the strip's interactive controls.
     *
     * @param[in] enabled  True to enable.
     */
    void setControlsEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Get the viewport index this strip controls.
     *
     * @return Viewport index.
     */
    int viewportIndex() const { return m_viewportIdx; }

    //=========================================================================================================
    /**
     * Get the current slider value.
     *
     * @return Current time index.
     */
    int currentValue() const;

    //=========================================================================================================
    /**
     * Get whether playback is active.
     *
     * @return True if playing.
     */
    bool isPlaying() const { return m_playing; }

    //=========================================================================================================
    /**
     * Stop playback and reset the play button text.
     */
    void stopPlayback();

signals:
    /** Emitted when the slider value changes. */
    void sliderValueChanged(int viewportIdx, int value);

    /** Emitted when play/pause is toggled. */
    void playToggled(int viewportIdx, bool playing);

    /** Emitted when step backward is clicked. */
    void stepBackward(int viewportIdx);

    /** Emitted when step forward is clicked. */
    void stepForward(int viewportIdx);

private:
    int m_viewportIdx;
    bool m_playing = false;
    QPushButton *m_stepBackBtn = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_stepFwdBtn = nullptr;
    QSlider *m_slider = nullptr;
    QLabel *m_timeLabel = nullptr;
};

#endif // VIEWPORTTIMESTRIP_H
