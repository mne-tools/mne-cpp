//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     svgwriter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the SvgWriter class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "svgwriter.h"

#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>
#include <QFile>
#include <QGradient>
#include <QImage>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPixmap>
#include <QXmlStreamWriter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr std::string_view kSvgNamespace = "http://www.w3.org/2000/svg";
constexpr std::string_view kXlinkNamespace = "http://www.w3.org/1999/xlink";

std::string number(double value)
{
    return std::format("{:.8g}", value);
}

std::string colorName(const QColor& color)
{
    return std::format("#{:02x}{:02x}{:02x}", color.red(), color.green(), color.blue());
}

std::string pathData(const QPainterPath& path)
{
    std::string d;
    const int count = path.elementCount();
    for (int i = 0; i < count; ++i) {
        const auto e = path.elementAt(i);
        if (e.isMoveTo()) {
            d += std::format("M{} {}", number(e.x), number(e.y));
        } else if (e.isLineTo()) {
            d += std::format("L{} {}", number(e.x), number(e.y));
        } else if (e.isCurveTo() && i + 2 < count) {
            const auto c2 = path.elementAt(i + 1);
            const auto to = path.elementAt(i + 2);
            d += std::format("C{} {} {} {} {} {}", number(e.x), number(e.y), number(c2.x), number(c2.y), number(to.x), number(to.y));
            i += 2;
        }
    }
    return d.empty() ? std::string("M0 0") : d;
}

// XML 1.0 forbids control characters other than tab, line feed and carriage return; drop them.
std::string xmlText(std::string text)
{
    std::erase_if(text, [](char c) {
        return static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r';
    });
    return text;
}

QColor brushColor(const QBrush& brush)
{
    const auto* gradient = brush.gradient();
    if (gradient && !gradient->stops().isEmpty()) {
        return gradient->stops().first().second;
    }
    return brush.color();
}

} // namespace

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
/**
 * Paint engine of SvgWriter; translates painter calls into SVG elements.
 */
class SvgPaintEngine : public QPaintEngine
{
public:
    SvgPaintEngine()
    : QPaintEngine(AllFeatures & ~(PerspectiveTransform | LinearGradientFill | RadialGradientFill | ConicalGradientFill | PatternBrush | BrushStroke | MaskedBrush | ObjectBoundingModeGradients | PorterDuff | BlendModes | RasterOpModes))
    {
    }

    using QPaintEngine::drawEllipse;
    using QPaintEngine::drawLines;
    using QPaintEngine::drawPolygon;
    using QPaintEngine::drawRects;

    bool begin(QPaintDevice* pdev) override;
    bool end() override;
    void updateState(const QPaintEngineState& newState) override;
    void drawPath(const QPainterPath& path) override;
    void drawPolygon(const QPointF* points, int pointCount, PolygonDrawMode mode) override;
    void drawLines(const QLineF* lines, int lineCount) override;
    void drawRects(const QRectF* rects, int rectCount) override;
    void drawEllipse(const QRectF& rect) override;
    void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) override;
    void drawImage(const QRectF& r, const QImage& image, const QRectF& sr, Qt::ImageConversionFlags flags) override;
    void drawTextItem(const QPointF& p, const QTextItem& textItem) override;

    Type type() const override
    {
        return User;
    }

private:
    void startElement(std::string_view name);
    void emptyElement(std::string_view name);
    void attribute(std::string_view name, std::string_view value);
    void endElement();

    void applyClip(Qt::ClipOperation operation, const QPainterPath& devicePath);
    void ensureGroup();
    void closeGroups();
    void writeColor(std::string_view paint, const QColor& color);
    void writeStrokeAttributes();
    void writeCosmetic();

    QXmlStreamWriter m_xml;
    std::unique_ptr<QFile> m_pFile;

    QPen m_pen;
    QBrush m_brush;
    QTransform m_transform;
    double m_opacity = 1.0;

    QPainterPath m_clipPath; // in device coordinates
    bool m_hasClip = false;
    bool m_clipEnabled = false;
    int m_clipCount = 0;

    bool m_clipDirty = true;
    bool m_styleDirty = true;
    bool m_outerOpen = false;
    bool m_innerOpen = false;
};

} // namespace mne::disp

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SvgPaintEngine::startElement(std::string_view name)
{
    m_xml.writeStartElement(QString::fromUtf8(name));
}

//=============================================================================================================

void SvgPaintEngine::emptyElement(std::string_view name)
{
    m_xml.writeEmptyElement(QString::fromUtf8(name));
}

//=============================================================================================================

void SvgPaintEngine::attribute(std::string_view name, std::string_view value)
{
    m_xml.writeAttribute(QString::fromUtf8(name), QString::fromUtf8(value));
}

//=============================================================================================================

void SvgPaintEngine::endElement()
{
    m_xml.writeEndElement();
}

//=============================================================================================================

bool SvgPaintEngine::begin(QPaintDevice* pdev)
{
    auto* writer = static_cast<SvgWriter*>(pdev);
    QIODevice* device = writer->m_pDevice;

    if (!writer->m_fileName.isEmpty()) {
        m_pFile = std::make_unique<QFile>(writer->m_fileName);
        if (!m_pFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            Log::warning("SvgWriter: cannot open {} for writing: {}", writer->m_fileName.toStdString(), m_pFile->errorString().toStdString());
            m_pFile.reset();
            return false;
        }
        device = m_pFile.get();
    } else if (!device) {
        Log::warning("SvgWriter: neither a file name nor an output device is set");
        return false;
    } else if ((!device->isOpen() && !device->open(QIODevice::WriteOnly)) || !device->isWritable()) {
        Log::warning("SvgWriter: the output device cannot be opened for writing");
        return false;
    }

    m_pen = QPen();
    m_brush = QBrush();
    m_transform = QTransform();
    m_opacity = 1.0;
    m_clipPath = QPainterPath();
    m_hasClip = false;
    m_clipEnabled = false;
    m_clipCount = 0;
    m_clipDirty = true;
    m_styleDirty = true;
    m_outerOpen = false;
    m_innerOpen = false;

    const auto size = writer->size();
    const auto viewBox = writer->viewBox();

    m_xml.setDevice(device);
    m_xml.setAutoFormatting(true);
    m_xml.setAutoFormattingIndent(1);
    m_xml.writeStartDocument();
    startElement("svg");
    m_xml.writeDefaultNamespace(QString::fromUtf8(kSvgNamespace));
    m_xml.writeNamespace(QString::fromUtf8(kXlinkNamespace), QString::fromUtf8("xlink"));
    attribute("version", "1.1");
    attribute("width", std::to_string(size.width()));
    attribute("height", std::to_string(size.height()));
    attribute("viewBox", std::format("{} {} {} {}", number(viewBox.x()), number(viewBox.y()), number(viewBox.width()), number(viewBox.height())));
    for (const auto& [name, text] : {std::pair{"title", writer->m_title}, std::pair{"desc", writer->m_description}}) {
        if (!text.isEmpty()) {
            startElement(name);
            m_xml.writeCharacters(QString::fromStdString(xmlText(text.toStdString())));
            endElement();
        }
    }
    return true;
}

//=============================================================================================================

bool SvgPaintEngine::end()
{
    closeGroups();
    endElement();
    m_xml.writeEndDocument();
    const bool ok = !m_xml.hasError();
    m_xml.setDevice(nullptr);
    if (m_pFile) {
        m_pFile->close();
        m_pFile.reset();
    }
    if (!ok) {
        Log::warning("SvgWriter: writing the SVG document failed");
    }
    return ok;
}

//=============================================================================================================

void SvgPaintEngine::updateState(const QPaintEngineState& newState)
{
    const auto flags = newState.state();
    if (flags & DirtyTransform) {
        m_transform = newState.transform();
        m_styleDirty = true;
    }
    if (flags & DirtyPen) {
        m_pen = newState.pen();
        m_styleDirty = true;
    }
    if (flags & DirtyBrush) {
        m_brush = newState.brush();
        m_styleDirty = true;
    }
    if (flags & DirtyOpacity) {
        m_opacity = newState.opacity();
        m_styleDirty = true;
    }
    if (flags & DirtyClipPath) {
        applyClip(newState.clipOperation(), newState.transform().map(newState.clipPath()));
    }
    if (flags & DirtyClipRegion) {
        QPainterPath path;
        path.addRegion(newState.clipRegion());
        applyClip(newState.clipOperation(), newState.transform().map(path));
    }
    if (flags & DirtyClipEnabled) {
        m_clipEnabled = newState.isClipEnabled();
        m_clipDirty = true;
    }
}

//=============================================================================================================

void SvgPaintEngine::applyClip(Qt::ClipOperation operation, const QPainterPath& devicePath)
{
    if (operation == Qt::NoClip) {
        m_hasClip = false;
        m_clipEnabled = false;
        m_clipPath = QPainterPath();
    } else {
        m_clipPath = (m_hasClip && operation == Qt::IntersectClip) ? m_clipPath.intersected(devicePath) : devicePath;
        m_hasClip = true;
        m_clipEnabled = true;
    }
    m_clipDirty = true;
}

//=============================================================================================================

void SvgPaintEngine::closeGroups()
{
    if (m_innerOpen) {
        endElement();
        m_innerOpen = false;
    }
    if (m_outerOpen) {
        endElement();
        m_outerOpen = false;
    }
}

//=============================================================================================================

void SvgPaintEngine::ensureGroup()
{
    // The clip group carries no transform, so its clip path is in device coordinates.
    if (m_clipDirty) {
        closeGroups();
        if (m_clipEnabled && m_hasClip) {
            const std::string id = std::format("clip{}", ++m_clipCount);
            startElement("clipPath");
            attribute("id", id);
            emptyElement("path");
            attribute("d", pathData(m_clipPath));
            if (m_clipPath.fillRule() == Qt::OddEvenFill) {
                attribute("clip-rule", "evenodd");
            }
            endElement();
            startElement("g");
            attribute("clip-path", std::format("url(#{})", id));
            m_outerOpen = true;
        }
        m_clipDirty = false;
        m_styleDirty = true;
    }
    if (m_styleDirty) {
        if (m_innerOpen) {
            endElement();
        }
        startElement("g");
        if (!m_transform.isIdentity()) {
            const auto& t = m_transform;
            attribute("transform", std::format("matrix({} {} {} {} {} {})", number(t.m11()), number(t.m12()), number(t.m21()), number(t.m22()), number(t.dx()), number(t.dy())));
        }
        if (m_brush.style() == Qt::NoBrush) {
            attribute("fill", "none");
        } else {
            writeColor("fill", brushColor(m_brush));
        }
        writeStrokeAttributes();
        m_innerOpen = true;
        m_styleDirty = false;
    }
}

//=============================================================================================================

void SvgPaintEngine::writeColor(std::string_view paint, const QColor& color)
{
    attribute(paint, colorName(color));
    const double alpha = color.alphaF() * m_opacity;
    if (alpha < 1.0) {
        attribute(std::string(paint) + "-opacity", number(alpha));
    }
}

//=============================================================================================================

void SvgPaintEngine::writeStrokeAttributes()
{
    if (m_pen.style() == Qt::NoPen || m_pen.brush().style() == Qt::NoBrush) {
        attribute("stroke", "none");
        return;
    }
    writeColor("stroke", brushColor(m_pen.brush()));
    const double width = m_pen.widthF() > 0.0 ? m_pen.widthF() : 1.0;
    attribute("stroke-width", number(width));

    const auto cap = m_pen.capStyle();
    attribute("stroke-linecap", cap == Qt::FlatCap ? "butt" : cap == Qt::RoundCap ? "round"
                                                                                  : "square");
    const auto join = m_pen.joinStyle();
    if (join == Qt::RoundJoin || join == Qt::BevelJoin) {
        attribute("stroke-linejoin", join == Qt::RoundJoin ? "round" : "bevel");
    } else {
        attribute("stroke-linejoin", "miter");
        attribute("stroke-miterlimit", number(std::max(1.0, static_cast<double>(m_pen.miterLimit()))));
    }

    if (m_pen.style() != Qt::SolidLine) {
        // Dash patterns are in units of the pen width.
        std::string dashes;
        for (const double dash : m_pen.dashPattern()) {
            dashes += (dashes.empty() ? "" : ",") + number(dash * width);
        }
        if (!dashes.empty()) {
            attribute("stroke-dasharray", dashes);
            if (m_pen.dashOffset() != 0.0) {
                attribute("stroke-dashoffset", number(m_pen.dashOffset() * width));
            }
        }
    }
}

//=============================================================================================================

void SvgPaintEngine::writeCosmetic()
{
    if (m_pen.style() != Qt::NoPen && m_pen.isCosmetic() && !m_transform.isIdentity()) {
        attribute("vector-effect", "non-scaling-stroke");
    }
}

//=============================================================================================================

void SvgPaintEngine::drawPath(const QPainterPath& path)
{
    ensureGroup();
    emptyElement("path");
    attribute("d", pathData(path));
    if (path.fillRule() == Qt::OddEvenFill) {
        attribute("fill-rule", "evenodd");
    }
    writeCosmetic();
}

//=============================================================================================================

void SvgPaintEngine::drawPolygon(const QPointF* points, int pointCount, PolygonDrawMode mode)
{
    if (pointCount <= 0) {
        return;
    }
    std::string list;
    for (int i = 0; i < pointCount; ++i) {
        list += std::format("{}{},{}", i == 0 ? "" : " ", number(points[i].x()), number(points[i].y()));
    }
    ensureGroup();
    emptyElement(mode == PolylineMode ? "polyline" : "polygon");
    attribute("points", list);
    if (mode == PolylineMode) {
        attribute("fill", "none");
    } else if (mode == OddEvenMode) {
        attribute("fill-rule", "evenodd");
    }
    writeCosmetic();
}

//=============================================================================================================

void SvgPaintEngine::drawLines(const QLineF* lines, int lineCount)
{
    ensureGroup();
    for (int i = 0; i < lineCount; ++i) {
        emptyElement("line");
        attribute("x1", number(lines[i].x1()));
        attribute("y1", number(lines[i].y1()));
        attribute("x2", number(lines[i].x2()));
        attribute("y2", number(lines[i].y2()));
        writeCosmetic();
    }
}

//=============================================================================================================

void SvgPaintEngine::drawRects(const QRectF* rects, int rectCount)
{
    ensureGroup();
    for (int i = 0; i < rectCount; ++i) {
        const auto r = rects[i].normalized();
        emptyElement("rect");
        attribute("x", number(r.x()));
        attribute("y", number(r.y()));
        attribute("width", number(r.width()));
        attribute("height", number(r.height()));
        writeCosmetic();
    }
}

//=============================================================================================================

void SvgPaintEngine::drawEllipse(const QRectF& rect)
{
    ensureGroup();
    const auto r = rect.normalized();
    emptyElement("ellipse");
    attribute("cx", number(r.center().x()));
    attribute("cy", number(r.center().y()));
    attribute("rx", number(r.width() / 2.0));
    attribute("ry", number(r.height() / 2.0));
    writeCosmetic();
}

//=============================================================================================================

void SvgPaintEngine::drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr)
{
    drawImage(r, pm.toImage(), sr, Qt::AutoColor);
}

//=============================================================================================================

void SvgPaintEngine::drawImage(const QRectF& r, const QImage& image, const QRectF& sr, Qt::ImageConversionFlags /*flags*/)
{
    if (image.isNull() || r.isEmpty()) {
        return;
    }
    const auto source = sr.toAlignedRect();
    const auto part = (source == image.rect() || source.isEmpty()) ? image : image.copy(source);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    if (!part.save(&buffer, "PNG")) {
        Log::warning("SvgWriter: cannot encode a {}x{} image as PNG", part.width(), part.height());
        return;
    }

    ensureGroup();
    emptyElement("image");
    attribute("x", number(r.x()));
    attribute("y", number(r.y()));
    attribute("width", number(r.width()));
    attribute("height", number(r.height()));
    attribute("preserveAspectRatio", "none");
    if (m_opacity < 1.0) {
        attribute("opacity", number(m_opacity));
    }
    m_xml.writeAttribute(QString::fromUtf8(kXlinkNamespace), QString::fromUtf8("href"),
                         QString::fromLatin1("data:image/png;base64," + buffer.data().toBase64()));
}

//=============================================================================================================

void SvgPaintEngine::drawTextItem(const QPointF& p, const QTextItem& textItem)
{
    const std::string text = xmlText(textItem.text().toStdString());
    if (text.empty()) {
        return;
    }
    ensureGroup();

    const auto font = textItem.font();
    const auto* writer = static_cast<const SvgWriter*>(paintDevice());
    const double pixelSize = font.pixelSize() > 0 ? font.pixelSize() : font.pointSizeF() * writer->resolution() / 72.0;

    startElement("text");
    attribute("xml:space", "preserve");
    attribute("x", number(p.x()));
    attribute("y", number(p.y()));
    attribute("font-family", font.family().toStdString());
    attribute("font-size", number(pixelSize));
    attribute("font-weight", std::to_string(static_cast<int>(font.weight())));
    if (font.italic()) {
        attribute("font-style", "italic");
    }
    std::string decoration;
    for (const auto& [set, name] : {std::pair{font.underline(), "underline"}, std::pair{font.overline(), "overline"},
                                    std::pair{font.strikeOut(), "line-through"}}) {
        if (set) {
            decoration += (decoration.empty() ? "" : " ") + std::string(name);
        }
    }
    if (!decoration.empty()) {
        attribute("text-decoration", decoration);
    }
    // Text is painted with the pen colour.
    if (m_pen.style() == Qt::NoPen) {
        attribute("fill", "none");
    } else {
        writeColor("fill", brushColor(m_pen.brush()));
    }
    attribute("stroke", "none");
    m_xml.writeCharacters(QString::fromStdString(text));
    endElement();
}

//=============================================================================================================

SvgWriter::SvgWriter()
: m_pEngine(std::make_unique<SvgPaintEngine>())
{
}

//=============================================================================================================

SvgWriter::~SvgWriter() = default;

//=============================================================================================================

void SvgWriter::setFileName(const QString& fileName)
{
    m_fileName = fileName;
    m_pDevice = nullptr;
}

//=============================================================================================================

QString SvgWriter::fileName() const
{
    return m_fileName;
}

//=============================================================================================================

void SvgWriter::setOutputDevice(QIODevice* device)
{
    m_pDevice = device;
    m_fileName.clear();
}

//=============================================================================================================

QIODevice* SvgWriter::outputDevice() const
{
    return m_pDevice;
}

//=============================================================================================================

void SvgWriter::setSize(const QSize& size)
{
    m_size = size;
}

//=============================================================================================================

QSize SvgWriter::size() const
{
    return m_size.isValid() ? m_size : m_viewBox.size().toSize();
}

//=============================================================================================================

void SvgWriter::setViewBox(const QRectF& viewBox)
{
    m_viewBox = viewBox;
}

//=============================================================================================================

void SvgWriter::setViewBox(const QRect& viewBox)
{
    m_viewBox = QRectF(viewBox);
}

//=============================================================================================================

QRectF SvgWriter::viewBox() const
{
    return m_viewBox.isValid() ? m_viewBox : QRectF(QPointF(0.0, 0.0), QSizeF(size()));
}

//=============================================================================================================

void SvgWriter::setTitle(const QString& title)
{
    m_title = title;
}

//=============================================================================================================

QString SvgWriter::title() const
{
    return m_title;
}

//=============================================================================================================

void SvgWriter::setDescription(const QString& description)
{
    m_description = description;
}

//=============================================================================================================

QString SvgWriter::description() const
{
    return m_description;
}

//=============================================================================================================

void SvgWriter::setResolution(int dpi)
{
    m_resolution = qMax(1, dpi);
}

//=============================================================================================================

int SvgWriter::resolution() const
{
    return m_resolution;
}

//=============================================================================================================

QPaintEngine* SvgWriter::paintEngine() const
{
    return m_pEngine.get();
}

//=============================================================================================================

int SvgWriter::metric(PaintDeviceMetric metric) const
{
    const QSize s = size();
    switch (metric) {
        case PdmWidth:
            return s.width();
        case PdmHeight:
            return s.height();
        case PdmWidthMM:
            return qRound(s.width() * 25.4 / m_resolution);
        case PdmHeightMM:
            return qRound(s.height() * 25.4 / m_resolution);
        case PdmNumColors:
            return std::numeric_limits<int>::max();
        case PdmDepth:
            return 32;
        case PdmDpiX:
        case PdmDpiY:
        case PdmPhysicalDpiX:
        case PdmPhysicalDpiY:
            return m_resolution;
        case PdmDevicePixelRatio:
            return 1;
        case PdmDevicePixelRatioScaled:
            return qRound(QPaintDevice::devicePixelRatioFScale());
        default:
            return QPaintDevice::metric(metric);
    }
}
