//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     rawsettings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     January, 2014
 * @version  2.1.0
 * @brief    Contains all application settings.
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
, m_qSettings("mne-cpp","mne_analyze_studio_fiff_browser")
{
    init();
}


//*************************************************************************************************************

RawSettings::~RawSettings()
{
}


//*************************************************************************************************************

void RawSettings::writeMainWindow()
{
    m_qSettings.beginGroup("MainWindow");
    m_qSettings.setValue("size", QSize(m_mainwindow_size_w, m_mainwindow_size_h));
    m_qSettings.setValue("position", QPoint(m_mainwindow_position_x, m_mainwindow_position_y));
    m_qSettings.endGroup();
}


//*************************************************************************************************************

void RawSettings::writeEventColors()
{
    m_qSettings.beginGroup("EventDesignParameters");
    m_qSettings.setValue("event_color_default", QVariant(m_event_color_default));
    m_qSettings.setValue("event_color_1",       QVariant(m_event_color_1));
    m_qSettings.setValue("event_color_2",       QVariant(m_event_color_2));
    m_qSettings.setValue("event_color_3",       QVariant(m_event_color_3));
    m_qSettings.setValue("event_color_4",       QVariant(m_event_color_4));
    m_qSettings.setValue("event_color_5",       QVariant(m_event_color_5));
    m_qSettings.setValue("event_color_32",      QVariant(m_event_color_32));
    m_qSettings.setValue("event_color_998",     QVariant(m_event_color_998));
    m_qSettings.setValue("event_color_999",     QVariant(m_event_color_999));
    m_qSettings.endGroup();
}


//*************************************************************************************************************

void RawSettings::writeDataMarker()
{
    m_qSettings.beginGroup("DataMarker");
    m_qSettings.setValue("data_marker_color", QVariant(m_data_marker_color));
    m_qSettings.endGroup();
}


//*************************************************************************************************************

void RawSettings::write()
{
    // Split into per-group helpers — works around an Apple clang 17
    // (clang-1700.6.3.2) frontend crash (Trace/BPT trap: 5) that triggers
    // when the entire write() body is processed as one large function.
    writeMainWindow();
    writeEventColors();
    writeDataMarker();
}


//*************************************************************************************************************

void RawSettings::init()
{
    m_mainwindow_size_w = MAINWINDOW_WINDOW_SIZE_W;
    m_mainwindow_size_h = MAINWINDOW_WINDOW_SIZE_H;
    m_mainwindow_position_x = MAINWINDOW_WINDOW_POSITION_X;
    m_mainwindow_position_y = MAINWINDOW_WINDOW_POSITION_Y;

    m_event_color_default = Qt::black;
    m_event_color_1 = Qt::black;
    m_event_color_2 = Qt::magenta;
    m_event_color_3 = Qt::green;
    m_event_color_4 = Qt::red;
    m_event_color_5 = Qt::cyan;
    m_event_color_32 = Qt::yellow;
    m_event_color_998 = Qt::darkBlue;
    m_event_color_999 = Qt::darkCyan;
    m_data_marker_color = QColor (93,177,47); //green
    //m_data_marker_color = QColor (227,6,19); //red
}
