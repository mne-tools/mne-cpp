//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qttheme.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The viz::Theme of the toolkit-free panels applied to Qt Widgets.
 */

#ifndef VIZ_QT_QTTHEME_H
#define VIZ_QT_QTTHEME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include <mne/viz/figures/theme.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

class QWidget;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @param[in] theme   Colours, fonts and metrics of the panels.
 *
 * @return A Qt style sheet that gives group boxes, fields, spin boxes, combo boxes and buttons the look of
 *         viz::FormView, so panels look alike in every backend.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT QString themeStyleSheet(const viz::Theme& theme);

//=============================================================================================================
/**
 * Gives a widget and its children the theme: style sheet, font and palette.
 *
 * @param[in] widget   Widget, e.g. a PanelWidget.
 * @param[in] theme    Colours, fonts and metrics of the panels.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT void applyTheme(QWidget* widget, const viz::Theme& theme);

} // namespace mne::viz::qt

#endif // VIZ_QT_QTTHEME_H
