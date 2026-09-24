//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rawview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Definition of the RawView class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawview.h"

using namespace MNEBROWSE;

//=============================================================================================================

RawView::RawView(const QString &sSettingsPath,
                 QWidget *parent,
                 Qt::WindowFlags f)
: DISPLIB::ChannelDataView(sSettingsPath, parent, f)
{
}

//=============================================================================================================

RawView::~RawView() = default;

