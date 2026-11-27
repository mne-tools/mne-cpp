//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     figureexporter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Definition of the FigureExporter class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "figureexporter.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <cstdlib>
#include <format>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

std::atomic<int> figureCount{0};

constexpr qreal kPixelRatio = 2.0;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::filesystem::path FigureExporter::directory()
{
    const char* dir = std::getenv("MNE_CPP_FIGURE_DIR");
    return dir ? mne::path_from_utf8(dir) : std::filesystem::path();
}

//=============================================================================================================

std::filesystem::path FigureExporter::save(QWidget& widget,
                                           const std::string& name,
                                           const std::string& caption,
                                           const QSize& size)
{
    if (directory().empty()) {
        return {};
    }
    widget.resize(size);
    widget.setAttribute(Qt::WA_DontShowOnScreen);
    widget.show();
    QApplication::processEvents();

    QImage image(size * kPixelRatio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(kPixelRatio);
    image.fill(Qt::white);
    QPainter painter(&image);
    widget.render(&painter);
    painter.end();
    widget.hide();
    return save(image, name, caption);
}

//=============================================================================================================

std::filesystem::path FigureExporter::save(const QImage& image,
                                           const std::string& name,
                                           const std::string& caption)
{
    const std::filesystem::path dir = directory();
    std::error_code error;
    if (dir.empty() || image.isNull() || (std::filesystem::create_directories(dir, error), error)) {
        return {};
    }
    std::string stem;
    for (const char c : name) {
        const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (safe) {
            stem += c;
        } else if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
            stem += '_'; // one '_' per character, not per UTF-8 byte
        }
    }
    const std::filesystem::path path = dir / mne::path_from_utf8(std::format("{:02}_{}.png", figureCount.fetch_add(1) + 1, stem));
    QImage figure = image.convertToFormat(QImage::Format_RGB32);
    figure.setText("Description", QString::fromStdString(caption));
    return figure.save(QString::fromStdString(mne::utf8_from_path(path)), "PNG") ? path : std::filesystem::path();
}
