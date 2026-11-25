//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scenepainter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of paintScene and QtTextMetrics.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scenepainter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QFontMetrics>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <variant>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

namespace
{

QColor toQColor(scene::Color c)
{
    return QColor(c.r, c.g, c.b, c.a);
}

QTransform toQTransform(const scene::Transform& t)
{
    return QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy);
}

QRectF toQRect(const scene::Rect& r)
{
    return QRectF(r.x, r.y, r.width, r.height);
}

QPointF toQPoint(scene::Point p)
{
    return QPointF(p.x, p.y);
}

QPen toQPen(const scene::Pen& pen)
{
    if (!pen.visible()) {
        return QPen(Qt::NoPen);
    }
    Qt::PenStyle style = Qt::SolidLine;
    switch (pen.style) {
        case scene::LineStyle::Dash:
            style = Qt::DashLine;
            break;
        case scene::LineStyle::Dot:
            style = Qt::DotLine;
            break;
        case scene::LineStyle::DashDot:
            style = Qt::DashDotLine;
            break;
        case scene::LineStyle::DashDotDot:
            style = Qt::DashDotDotLine;
            break;
        default:
            break;
    }
    const Qt::PenCapStyle cap = pen.cap == scene::LineCap::Flat ? Qt::FlatCap : pen.cap == scene::LineCap::Round ? Qt::RoundCap
                                                                                                                 : Qt::SquareCap;
    const Qt::PenJoinStyle join = pen.join == scene::LineJoin::Miter ? Qt::MiterJoin : pen.join == scene::LineJoin::Round ? Qt::RoundJoin
                                                                                                                          : Qt::BevelJoin;
    return QPen(QBrush(toQColor(pen.color)), pen.width, style, cap, join);
}

QBrush toQBrush(const scene::Brush& brush)
{
    switch (brush.style) {
        case scene::Brush::Style::Solid:
            return QBrush(toQColor(brush.color));
        case scene::Brush::Style::LinearGradient: {
            QLinearGradient gradient(toQPoint(brush.start), toQPoint(brush.end));
            for (const scene::GradientStop& stop : brush.stops) {
                gradient.setColorAt(stop.position, toQColor(stop.color));
            }
            return QBrush(gradient);
        }
        default:
            return QBrush(Qt::NoBrush);
    }
}

class Painter
{
public:
    Painter(QPainter& painter, const QFont& baseFont)
    : m_painter(painter)
    , m_base(painter.worldTransform())
    , m_baseOpacity(painter.opacity())
    , m_baseFont(baseFont)
    {
    }

    void draw(const scene::Command& command)
    {
        m_painter.save();
        m_painter.setRenderHint(QPainter::Antialiasing, command.antialias);
        m_painter.setOpacity(m_baseOpacity * command.opacity);
        for (const scene::Clip& clip : command.clips) {
            m_painter.setWorldTransform(toQTransform(clip.transform) * m_base);
            m_painter.setClipRect(toQRect(clip.rect), Qt::IntersectClip);
        }
        m_painter.setWorldTransform(toQTransform(command.transform) * m_base);
        std::visit([this](const auto& shape) { paint(shape); }, command.shape);
        m_painter.restore();
    }

private:
    void paint(const scene::LineShape& line)
    {
        m_painter.setPen(toQPen(line.pen));
        m_painter.drawLine(toQPoint(line.from), toQPoint(line.to));
    }

    void paint(const scene::PathShape& path)
    {
        if (path.cubic) {
            QPainterPath curve(toQPoint(path.points[0]));
            for (std::size_t i = 1; i + 2 < path.points.size(); i += 3) {
                curve.cubicTo(toQPoint(path.points[i]), toQPoint(path.points[i + 1]), toQPoint(path.points[i + 2]));
            }
            if (path.closed) {
                curve.closeSubpath();
            }
            m_painter.setPen(toQPen(path.pen));
            m_painter.setBrush(path.closed ? toQBrush(path.brush) : QBrush(Qt::NoBrush));
            m_painter.drawPath(curve);
            return;
        }
        QPolygonF polygon;
        polygon.reserve(static_cast<qsizetype>(path.points.size()));
        for (const scene::Point& p : path.points) {
            polygon.append(toQPoint(p));
        }
        m_painter.setPen(toQPen(path.pen));
        if (path.closed) {
            m_painter.setBrush(toQBrush(path.brush));
            m_painter.drawPolygon(polygon);
        } else {
            m_painter.setBrush(Qt::NoBrush);
            m_painter.drawPolyline(polygon);
        }
    }

    void paint(const scene::RectShape& rect)
    {
        m_painter.setPen(toQPen(rect.pen));
        m_painter.setBrush(toQBrush(rect.brush));
        if (rect.rx > 0.0 || rect.ry > 0.0) {
            m_painter.drawRoundedRect(toQRect(rect.rect), rect.rx, rect.ry);
        } else if (!rect.pen.visible()) {
            m_painter.fillRect(toQRect(rect.rect), toQBrush(rect.brush));
        } else {
            m_painter.drawRect(toQRect(rect.rect));
        }
    }

    void paint(const scene::EllipseShape& ellipse)
    {
        m_painter.setPen(toQPen(ellipse.pen));
        m_painter.setBrush(toQBrush(ellipse.brush));
        m_painter.drawEllipse(toQPoint(ellipse.center), ellipse.rx, ellipse.ry);
    }

    void paint(const scene::TextShape& text)
    {
        m_painter.setPen(toQColor(text.color));
        m_painter.setFont(toQFont(text.font, m_baseFont));
        const QString string = QString::fromStdString(text.text);
        if (scene::has_flag(text.align, scene::Align::Baseline)) {
            m_painter.drawText(QPointF(text.box.x, text.box.y), string);
        } else {
            m_painter.drawText(toQRect(text.box), static_cast<int>(text.align), string);
        }
    }

    void paint(const scene::ImageShape& shape)
    {
        const scene::Image& image = *shape.image;
        const QImage view(image.rgba.data(), image.width, image.height, image.width * 4, QImage::Format_RGBA8888);
        m_painter.setRenderHint(QPainter::SmoothPixmapTransform, shape.smooth);
        m_painter.drawImage(toQRect(shape.target), view);
    }

    QPainter& m_painter;
    QTransform m_base;
    qreal m_baseOpacity;
    QFont m_baseFont;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

void mne::disp::paintScene(QPainter& painter, const scene::DisplayList& list, const QFont& baseFont)
{
    if (list.background) {
        painter.fillRect(QRectF(0, 0, list.width, list.height), toQColor(*list.background));
    }
    Painter backend(painter, baseFont);
    for (const scene::Command& command : list.commands) {
        backend.draw(command);
    }
}

//=============================================================================================================

QFont mne::disp::toQFont(const scene::Font& font, const QFont& baseFont)
{
    QFont result = baseFont;
    if (!font.family.empty()) {
        result.setFamily(QString::fromStdString(font.family));
    }
    if (font.pixel_size > 0.0) {
        result.setPixelSize(qMax(1, qRound(font.pixel_size)));
    } else if (font.point_size > 0.0) {
        result.setPointSizeF(font.point_size);
    }
    result.setBold(font.bold);
    result.setItalic(font.italic);
    return result;
}

//=============================================================================================================

scene::Font mne::disp::toSceneFont(const QFont& font)
{
    scene::Font result;
    if (font.pixelSize() > 0) {
        result.pixel_size = font.pixelSize();
    } else {
        result.point_size = font.pointSizeF();
    }
    result.bold = font.bold();
    result.italic = font.italic();
    return result;
}

//=============================================================================================================

scene::Color mne::disp::toSceneColor(const QColor& color)
{
    // An invalid colour paints opaque black in QPainter, and its components say so
    const QColor rgb = color.isValid() ? color.toRgb() : color;
    return {static_cast<std::uint8_t>(rgb.red()), static_cast<std::uint8_t>(rgb.green()), static_cast<std::uint8_t>(rgb.blue()),
            static_cast<std::uint8_t>(rgb.alpha())};
}

//=============================================================================================================

scene::LineStyle mne::disp::toSceneLineStyle(Qt::PenStyle style)
{
    switch (style) {
        case Qt::NoPen:
            return scene::LineStyle::None;
        case Qt::DashLine:
            return scene::LineStyle::Dash;
        case Qt::DotLine:
            return scene::LineStyle::Dot;
        case Qt::DashDotLine:
            return scene::LineStyle::DashDot;
        case Qt::DashDotDotLine:
            return scene::LineStyle::DashDotDot;
        default:
            return scene::LineStyle::Solid;
    }
}

//=============================================================================================================

QtTextMetrics::QtTextMetrics(const QFont& baseFont)
: m_baseFont(baseFont)
{
}

//=============================================================================================================

double QtTextMetrics::pixel_size(const scene::Font& font) const
{
    const QFont qfont = toQFont(font, m_baseFont);
    return qfont.pixelSize() > 0 ? qfont.pixelSize() : QFontInfo(qfont).pixelSize();
}

//=============================================================================================================

double QtTextMetrics::advance(const scene::Font& font, std::string_view text) const
{
    return QFontMetrics(toQFont(font, m_baseFont)).horizontalAdvance(QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())));
}

//=============================================================================================================

double QtTextMetrics::ascent(const scene::Font& font) const
{
    return QFontMetrics(toQFont(font, m_baseFont)).ascent();
}

//=============================================================================================================

double QtTextMetrics::descent(const scene::Font& font) const
{
    return QFontMetrics(toQFont(font, m_baseFont)).descent();
}

//=============================================================================================================

double QtTextMetrics::height(const scene::Font& font) const
{
    return QFontMetrics(toQFont(font, m_baseFont)).height();
}
