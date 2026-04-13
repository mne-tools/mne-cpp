//=============================================================================================================
/**
 * @file     rawsettings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
 * @brief    In this RawSettings class all mne_browse settings are managed. In order to store settings
 *           from the last mne_browse session, the class uses the QtSettings class of Qt. [1]
 *           Using QSettings, the entries are stored locally in a OS-specific place from where they are
 *           loaded at each start of mne_browse if they were already set.
 *
 *
 *           [1] http://qt-project.org/doc/qt-5/QSettings.html
 *
 */
#ifndef RAWSETTINGS_H
#define RAWSETTINGS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "info.h"
#include "types.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSettings>
#include <QSize>
#include <QColor>
#include <QPoint>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace RawSettingsConstants {

//MainWindow
inline constexpr int MAINWINDOW_WINDOW_SIZE_W = 1200;            ///< Width of MainWindow
inline constexpr int MAINWINDOW_WINDOW_SIZE_H = 800;             ///< Height of MainWindow
inline constexpr int MAINWINDOW_WINDOW_POSITION_X = 50;          ///< Initial window position x
inline constexpr int MAINWINDOW_WINDOW_POSITION_Y = 50;          ///< Initial window position y

//RawModel
inline constexpr int MODEL_WINDOW_SIZE = 4016;            ///< Length of data window to preload [samples]. Must satisfy (value + MODEL_NUM_FILTER_TAPS) = 2^n
inline constexpr int MODEL_RELOAD_POS = 2000;             ///< Distance from window edge triggering reload [samples]
inline constexpr int MODEL_MAX_WINDOWS = 3;               ///< Maximum number of preloaded windows
inline constexpr int MODEL_NUM_FILTER_TAPS = 80;           ///< Filter tap count for FFT zero-padding
inline constexpr int MODEL_MAX_NUM_FILTER_TAPS = 0;         ///< Maximum filter tap count

//RawDelegate — Layout
inline constexpr int DELEGATE_PLOT_HEIGHT = 40;           ///< Height of a single channel row [pixels]
inline constexpr int DELEGATE_DX = 1;                    ///< Pixels per sample → plot resolution
inline constexpr int DELEGATE_NHLINES = 6;               ///< Horizontal grid lines per row

//RawDelegate — Maximum channel amplitudes (by FiffChInfo type)
inline constexpr double DELEGATE_MAX_MEG_GRAD = 1e-10;    ///< kind=FIFFV_MEG_CH, unit=FIFF_UNIT_T_M
inline constexpr double DELEGATE_MAX_MEG_MAG = 1e-11;     ///< kind=FIFFV_MEG_CH, unit=FIFF_UNIT_T
inline constexpr double DELEGATE_MAX_EEG = 1e-4;         ///< kind=FIFFV_EEG_CH
inline constexpr double DELEGATE_MAX_EOG = 1e-3;         ///< kind=FIFFV_EOG_CH
inline constexpr double DELEGATE_MAX_STIM = 5.0;         ///< kind=FIFFV_STIM_CH

//Event markers
inline constexpr int EVENT_MARKER_WIDTH = 3;              ///< Event marker width [pixels]
inline constexpr int EVENT_MARKER_OPACITY = 110;          ///< Event marker opacity [0..255]

//Data markers
inline constexpr int DATA_MARKER_WIDTH = 3;               ///< Data marker width [pixels]
inline constexpr int DATA_MARKER_OPACITY = 200;           ///< Data marker opacity [0..255]
inline constexpr int DATA_MARKER_INITIAL_X = 74;           ///< Initial x position after file load [pixels]
inline constexpr int DATA_MARKER_LABEL_V_OFFSET = 20;       ///< Label offset above marker line [pixels]

//RawDelegate — Initial/fallback scaling
inline constexpr double DELEGATE_INITIAL_MAX_VALUE = 65530.0; ///< Placeholder max before file load
inline constexpr double DELEGATE_FALLBACK_SCALE = 1e-9;      ///< Fallback for unknown channel types

//Keyboard navigation
inline constexpr int RAWVIEW_KEYBOARD_SCROLL_STEP = 25;         ///< Pixels per Left/Right key press

//Filter design
inline constexpr double FILTER_DEFAULT_TRANS_BW_RATIO = 0.2; ///< Transition-band width as fraction of Nyquist

//Default channel amplitude scales (for scale map initialization)
inline constexpr double DELEGATE_SCALE_MEG_GRAD = 400e-15 * 100; ///< fT/m (×100 because FIFF stores fT/cm)
inline constexpr double DELEGATE_SCALE_MEG_MAG = 1.2e-12;
inline constexpr double DELEGATE_SCALE_EEG = 30e-6;
inline constexpr double DELEGATE_SCALE_EOG = 150e-6;
inline constexpr double DELEGATE_SCALE_EMG = 1e-3;
inline constexpr double DELEGATE_SCALE_ECG = 1e-3;
inline constexpr double DELEGATE_SCALE_MISC = 1.0;
inline constexpr double DELEGATE_SCALE_STIM = 5.0;

} // namespace RawSettingsConstants

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * DECLARE CLASS RawSettings
 */
class RawSettings : public QObject
{
    Q_OBJECT

public:
    RawSettings(QObject *parent = 0);

    ~RawSettings();

    QColor m_event_color_default, m_event_color_1, m_event_color_2, m_event_color_3, m_event_color_4, m_event_color_5,
        m_event_color_32, m_event_color_998, m_event_color_999, m_data_marker_color;

    int m_mainwindow_size_w, m_mainwindow_size_h, m_mainwindow_position_x, m_mainwindow_position_y;

    //=========================================================================================================
    /**
     * write writes all the application settings values from the macros
     */
    void write();

private:
    QSettings m_qSettings;  /**< QSettings object that initializes all the RawSettings */

    //=========================================================================================================
    /**
     * init inits all the application settings values
     */
    void init();
};

} //NAMESPACE

#endif // RAWSETTINGS_H
