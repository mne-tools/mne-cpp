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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QAtomicInt>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QWidget>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

QAtomicInt figureCount;

constexpr qreal kPixelRatio = 2.0;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString FigureExporter::directory()
{
    return qEnvironmentVariable("MNE_CPP_FIGURE_DIR");
}

//=============================================================================================================

QString FigureExporter::save(QWidget& widget,
                             const QString& name,
                             const QString& caption,
                             const QSize& size)
{
    if (directory().isEmpty()) {
        return QString();
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

QString FigureExporter::save(const QImage& image,
                             const QString& name,
                             const QString& caption)
{
    const QString dir = directory();
    if (dir.isEmpty() || image.isNull() || !QDir().mkpath(dir)) {
        return QString();
    }
    static const QRegularExpression unsafe("[^A-Za-z0-9_-]");
    const QString stem = QString(name).replace(unsafe, "_");
    const QString path = QDir(dir).filePath(QString("%1_%2.png").arg(figureCount.fetchAndAddOrdered(1) + 1, 2, 10, QChar('0')).arg(stem));
    QImage figure = image.convertToFormat(QImage::Format_RGB32);
    figure.setText("Description", caption);
    return figure.save(path, "PNG") ? path : QString();
}
