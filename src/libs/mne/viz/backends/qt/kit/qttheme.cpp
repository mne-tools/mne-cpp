//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qttheme.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the Qt theme helpers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "qttheme.h"
#include <mne/viz/figures/font_library.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QByteArray>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <span>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

QString css(Color c)
{
    return c.a == 255 ? QColor(c.r, c.g, c.b).name() : QStringLiteral("rgba(%1, %2, %3, %4)").arg(c.r).arg(c.g).arg(c.b).arg(c.a);
}

QColor qcolor(Color c)
{
    return {c.r, c.g, c.b, c.a};
}

// The typefaces of viz::FormView, registered once so Qt widgets use them too
void registerBundledFonts()
{
    static const bool registered = [] {
        for (const std::span<const unsigned char> file : FontLibrary::bundled_files()) {
            QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(reinterpret_cast<const char*>(file.data()), static_cast<qsizetype>(file.size())));
        }
        return true;
    }();
    static_cast<void>(registered);
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

QString mne::viz::qt::themeStyleSheet(const Theme& t)
{
    const int control = static_cast<int>(t.control_height);
    // Borders take two of the control height, the rest is text and padding
    return QStringLiteral(R"(
QWidget[vizPanel="true"] {
    background: %13;
}
QGroupBox {
    background: %1;
    border: 1px solid %2;
    border-radius: %3px;
    margin-top: %4px;
    padding: 8px 12px 8px 12px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 2px;
    padding: 0 0 4px 0;
    color: %5;
    font-size: %6px;
    font-weight: 600;
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background: %7;
    color: %8;
    border: 1px solid %9;
    border-radius: 4px;
    min-height: %10px;
    padding: 0 8px;
    selection-background-color: %11;
    selection-color: %12;
}
QSpinBox, QDoubleSpinBox {
    padding: 0 2px 0 8px;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border: 1px solid %11;
}
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    color: %5;
}
QPushButton {
    background: %11;
    color: %12;
    border: none;
    border-radius: 4px;
    min-height: %10px;
    padding: 0 16px;
    font-weight: 600;
}
QPushButton:disabled {
    background: %9;
}
QToolButton {
    background: %7;
    border: 1px solid %9;
    border-radius: 4px;
    min-height: %10px;
    min-width: %10px;
}
)")
        .arg(css(t.surface), css(t.border))
        .arg(static_cast<int>(t.corner_radius))
        .arg(static_cast<int>(t.small_font_size * 1.6))
        .arg(css(t.muted_text))
        .arg(static_cast<int>(t.small_font_size))
        .arg(css(t.field), css(t.text), css(t.field_border))
        .arg(control - 2)
        .arg(css(t.accent), css(t.accent_text))
        .arg(css(t.background));
}

//=============================================================================================================

void mne::viz::qt::applyTheme(QWidget* widget, const Theme& theme)
{
    if (!widget) {
        return;
    }
    registerBundledFonts();
    QFont font = widget->font();
    if (!theme.font_family.empty()) {
        font.setFamily(QString::fromStdString(theme.font_family));
    }
    font.setPixelSize(static_cast<int>(theme.font_size));
    widget->setFont(font);

    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, qcolor(theme.background));
    palette.setColor(QPalette::WindowText, qcolor(theme.text));
    palette.setColor(QPalette::Base, qcolor(theme.field));
    palette.setColor(QPalette::Text, qcolor(theme.text));
    palette.setColor(QPalette::Button, qcolor(theme.field));
    palette.setColor(QPalette::ButtonText, qcolor(theme.text));
    palette.setColor(QPalette::Highlight, qcolor(theme.accent));
    palette.setColor(QPalette::HighlightedText, qcolor(theme.accent_text));
    palette.setColor(QPalette::PlaceholderText, qcolor(theme.muted_text));
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    widget->setProperty("vizPanel", true);
    widget->setStyleSheet(themeStyleSheet(theme));
}
