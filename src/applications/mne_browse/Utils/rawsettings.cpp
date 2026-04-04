//=============================================================================================================
/**
 * @file     rawsettings.cpp
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
 * @brief    Contains all application settings.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawsettings.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawSettings::RawSettings(QObject *parent)
: QObject(parent)
, m_qSettings("mne-cpp","mne_browse")
{
    init();
}


//*************************************************************************************************************

RawSettings::~RawSettings()
{
}


//*************************************************************************************************************

void RawSettings::write()
{
    //Window settings
    m_qSettings.beginGroup("MainWindow");

        m_qSettings.setValue("size",QSize(m_mainwindow_size_w, m_mainwindow_size_h));
        m_qSettings.setValue("position",QPoint(m_mainwindow_position_x, m_mainwindow_position_y));

    m_qSettings.endGroup();

    //EventDesignParameters
    m_qSettings.beginGroup("EventDesignParameters");

        //Event colors
        QVariant variant;
        variant = m_event_color_default;
        m_qSettings.setValue("event_color_default",variant);

        variant = m_event_color_1;
        m_qSettings.setValue("event_color_1",variant);

        variant = m_event_color_2;
        m_qSettings.setValue("event_color_2",variant);

        variant = m_event_color_3;
        m_qSettings.setValue("event_color_3",variant);

        variant = m_event_color_4;
        m_qSettings.setValue("event_color_4",variant);

        variant = m_event_color_5;
        m_qSettings.setValue("event_color_5",variant);

        variant = m_event_color_32;
        m_qSettings.setValue("event_color_32",variant);

        variant = m_event_color_998;
        m_qSettings.setValue("event_color_998",variant);

        variant = m_event_color_999;
        m_qSettings.setValue("event_color_999",variant);

    m_qSettings.endGroup();

    //Data window marker
    m_qSettings.beginGroup("DataMarker");

        //data marker color and width colors
        variant = m_data_marker_color;
        m_qSettings.setValue("data_marker_color",variant);

    m_qSettings.endGroup();
}


//*************************************************************************************************************

void RawSettings::init()
{
    //Window settings - load from QSettings if previously stored, otherwise use defaults
    m_qSettings.beginGroup("MainWindow");
    QSize storedSize = m_qSettings.value("size", QSize(MAINWINDOW_WINDOW_SIZE_W, MAINWINDOW_WINDOW_SIZE_H)).toSize();
    m_mainwindow_size_w = storedSize.width();
    m_mainwindow_size_h = storedSize.height();
    QPoint storedPos = m_qSettings.value("position", QPoint(MAINWINDOW_WINDOW_POSITION_X, MAINWINDOW_WINDOW_POSITION_Y)).toPoint();
    m_mainwindow_position_x = storedPos.x();
    m_mainwindow_position_y = storedPos.y();
    m_qSettings.endGroup();

    //Event colors - load from QSettings if previously stored, otherwise use defaults
    m_qSettings.beginGroup("EventDesignParameters");
    m_event_color_default = m_qSettings.value("event_color_default", QColor(Qt::black)).value<QColor>();
    m_event_color_1 = m_qSettings.value("event_color_1", QColor(Qt::black)).value<QColor>();
    m_event_color_2 = m_qSettings.value("event_color_2", QColor(Qt::magenta)).value<QColor>();
    m_event_color_3 = m_qSettings.value("event_color_3", QColor(Qt::green)).value<QColor>();
    m_event_color_4 = m_qSettings.value("event_color_4", QColor(Qt::red)).value<QColor>();
    m_event_color_5 = m_qSettings.value("event_color_5", QColor(Qt::cyan)).value<QColor>();
    m_event_color_32 = m_qSettings.value("event_color_32", QColor(Qt::yellow)).value<QColor>();
    m_event_color_998 = m_qSettings.value("event_color_998", QColor(Qt::darkBlue)).value<QColor>();
    m_event_color_999 = m_qSettings.value("event_color_999", QColor(Qt::darkCyan)).value<QColor>();
    m_qSettings.endGroup();

    //Data marker color
    m_qSettings.beginGroup("DataMarker");
    m_data_marker_color = m_qSettings.value("data_marker_color", QColor(93,177,47)).value<QColor>();
    m_qSettings.endGroup();
}
