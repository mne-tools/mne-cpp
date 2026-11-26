//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp_svg_writer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for SvgWriter: well-formed SVG with the painted primitives, text, images and escaping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/plots/helpers/svgwriter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

struct SvgElement
{
    QString name;
    QXmlStreamAttributes attributes;
    QString text;
};

// Parses @p data; fails the calling test when it is not well-formed XML.
QList<SvgElement> parse(const QByteArray& data, bool* ok)
{
    QList<SvgElement> elements;
    QXmlStreamReader reader(data);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            elements.append({reader.name().toString(), reader.attributes(), QString()});
        } else if (reader.isCharacters() && !reader.isWhitespace() && !elements.isEmpty()) {
            elements.last().text += reader.text();
        }
    }
    *ok = !reader.hasError();
    if (reader.hasError()) {
        qWarning() << "XML error:" << reader.errorString() << "at line" << reader.lineNumber();
    }
    return elements;
}

QList<SvgElement> named(const QList<SvgElement>& elements, const QString& name)
{
    QList<SvgElement> result;
    for (const SvgElement& e : elements) {
        if (e.name == name) {
            result.append(e);
        }
    }
    return result;
}

} // namespace

//=============================================================================================================
/**
 * Tests the SVG output of SvgWriter.
 */
class TestDispSvgWriter : public QObject
{
    Q_OBJECT

private slots:
    void writesPrimitives();
    void writesAndEscapesText();
    void writesImagesClipsAndTransforms();
    void writesFile();
    void rendersWidget();
    void failsWithoutOutput();
};

//=============================================================================================================

void TestDispSvgWriter::writesPrimitives()
{
    QBuffer buffer;
    SvgWriter svg;
    svg.setOutputDevice(&buffer);
    svg.setSize(QSize(200, 100));
    QCOMPARE(svg.viewBox(), QRectF(0, 0, 200, 100));

    QPainter painter(&svg);
    QVERIFY(painter.isActive());
    painter.setPen(QPen(QColor(0, 0, 255), 2.0, Qt::DashLine, Qt::RoundCap, Qt::BevelJoin));
    painter.setBrush(QColor(255, 0, 0));
    painter.drawRect(QRectF(10, 10, 30, 20));
    painter.drawEllipse(QRectF(50, 10, 40, 20));
    painter.setPen(QPen(QColor(0, 128, 0, 128), 1.5));
    painter.drawLine(QPointF(0, 0), QPointF(100, 50));
    const QPointF points[3] = {QPointF(0, 90), QPointF(50, 60), QPointF(100, 90)};
    painter.drawPolyline(points, 3);
    painter.drawPolygon(points, 3);
    QPainterPath path;
    path.moveTo(120, 80);
    path.cubicTo(130, 20, 170, 20, 180, 80);
    path.closeSubpath();
    painter.drawPath(path);
    QVERIFY(painter.end());

    bool ok = false;
    const QList<SvgElement> elements = parse(buffer.data(), &ok);
    QVERIFY(ok);
    QVERIFY(!elements.isEmpty());
    QCOMPARE(elements.first().name, QStringLiteral("svg"));
    QCOMPARE(elements.first().attributes.value("width").toString(), QStringLiteral("200"));
    QCOMPARE(elements.first().attributes.value("height").toString(), QStringLiteral("100"));
    QCOMPARE(elements.first().attributes.value("viewBox").toString(), QStringLiteral("0 0 200 100"));

    const QList<SvgElement> rects = named(elements, "rect");
    QCOMPARE(rects.size(), 1);
    QCOMPARE(rects.first().attributes.value("x").toString(), QStringLiteral("10"));
    QCOMPARE(rects.first().attributes.value("width").toString(), QStringLiteral("30"));
    const QList<SvgElement> ellipses = named(elements, "ellipse");
    QCOMPARE(ellipses.size(), 1);
    QCOMPARE(ellipses.first().attributes.value("cx").toString(), QStringLiteral("70"));
    QCOMPARE(ellipses.first().attributes.value("ry").toString(), QStringLiteral("10"));
    QCOMPARE(named(elements, "line").size(), 1);
    QCOMPARE(named(elements, "polyline").size(), 1);
    QCOMPARE(named(elements, "polyline").first().attributes.value("fill").toString(), QStringLiteral("none"));
    QCOMPARE(named(elements, "polygon").size(), 1);
    QCOMPARE(named(elements, "polygon").first().attributes.value("points").toString(), QStringLiteral("0,90 50,60 100,90"));
    const QList<SvgElement> paths = named(elements, "path");
    QCOMPARE(paths.size(), 1);
    QVERIFY(paths.first().attributes.value("d").toString().startsWith(QStringLiteral("M120 80C130 20 170 20 180 80")));

    // Style groups carry the pen and brush
    bool dashedRedGroup = false;
    bool translucentGreenGroup = false;
    for (const SvgElement& g : named(elements, "g")) {
        const QXmlStreamAttributes& a = g.attributes;
        if (a.value("fill") == QStringLiteral("#ff0000") && a.value("stroke") == QStringLiteral("#0000ff") && a.value("stroke-width") == QStringLiteral("2") && a.value("stroke-linecap") == QStringLiteral("round") && a.value("stroke-linejoin") == QStringLiteral("bevel") && a.value("stroke-dasharray") == QStringLiteral("8,4")) {
            dashedRedGroup = true;
        }
        if (a.value("stroke") == QStringLiteral("#008000") && a.value("stroke-opacity").toDouble() > 0.49 && a.value("stroke-opacity").toDouble() < 0.51 && !a.hasAttribute("stroke-dasharray")) {
            translucentGreenGroup = true;
        }
    }
    QVERIFY(dashedRedGroup);
    QVERIFY(translucentGreenGroup);
}

//=============================================================================================================

void TestDispSvgWriter::writesAndEscapesText()
{
    const QString text = QStringLiteral("a < b & \"c\" > 'd'");

    QBuffer buffer;
    SvgWriter svg;
    svg.setOutputDevice(&buffer);
    svg.setSize(QSize(300, 50));
    svg.setTitle("Title <&>");
    svg.setDescription("Description & more");

    QPainter painter(&svg);
    QFont font(QStringLiteral("Helvetica"));
    font.setPixelSize(12);
    font.setWeight(QFont::Bold);
    font.setItalic(true);
    painter.setFont(font);
    painter.setPen(QColor(10, 20, 30));
    painter.drawText(QPointF(5, 30), text);
    QVERIFY(painter.end());

    const QByteArray data = buffer.data();
    QVERIFY(data.contains("a &lt; b &amp;"));
    QVERIFY(data.contains("Title &lt;&amp;&gt;"));

    bool ok = false;
    const QList<SvgElement> elements = parse(data, &ok);
    QVERIFY(ok);
    QCOMPARE(named(elements, "title").first().text, QStringLiteral("Title <&>"));
    QCOMPARE(named(elements, "desc").first().text, QStringLiteral("Description & more"));

    const QList<SvgElement> texts = named(elements, "text");
    QVERIFY(!texts.isEmpty());
    QString written;
    for (const SvgElement& t : texts) {
        written += t.text;
    }
    QCOMPARE(written, text);
    const QXmlStreamAttributes& a = texts.first().attributes;
    QCOMPARE(a.value("font-family").toString(), QStringLiteral("Helvetica"));
    QCOMPARE(a.value("font-size").toString(), QStringLiteral("12"));
    QCOMPARE(a.value("font-weight").toString(), QStringLiteral("700"));
    QCOMPARE(a.value("font-style").toString(), QStringLiteral("italic"));
    QCOMPARE(a.value("fill").toString(), QStringLiteral("#0a141e"));
    QCOMPARE(a.value("x").toString(), QStringLiteral("5"));
    QCOMPARE(a.value("y").toString(), QStringLiteral("30"));
}

//=============================================================================================================

void TestDispSvgWriter::writesImagesClipsAndTransforms()
{
    QBuffer buffer;
    SvgWriter svg;
    svg.setOutputDevice(&buffer);
    svg.setSize(QSize(100, 100));
    svg.setViewBox(QRect(0, 0, 50, 50));

    QImage image(4, 4, QImage::Format_ARGB32);
    image.fill(Qt::red);

    QPainter painter(&svg);
    painter.setClipRect(QRect(0, 0, 40, 40));
    painter.translate(10, 20);
    painter.rotate(90);
    painter.setOpacity(0.5);
    painter.drawImage(QRectF(0, 0, 8, 8), image);
    QVERIFY(painter.end());

    bool ok = false;
    const QList<SvgElement> elements = parse(buffer.data(), &ok);
    QVERIFY(ok);
    QCOMPARE(elements.first().attributes.value("viewBox").toString(), QStringLiteral("0 0 50 50"));

    const QList<SvgElement> clips = named(elements, "clipPath");
    QCOMPARE(clips.size(), 1);
    const QString clipId = clips.first().attributes.value("id").toString();
    bool clipUsed = false;
    bool transformed = false;
    for (const SvgElement& g : named(elements, "g")) {
        clipUsed |= g.attributes.value("clip-path") == QStringLiteral("url(#%1)").arg(clipId);
        transformed |= g.attributes.value("transform") == QStringLiteral("matrix(0 1 -1 0 10 20)");
    }
    QVERIFY(clipUsed);
    QVERIFY(transformed);

    const QList<SvgElement> images = named(elements, "image");
    QCOMPARE(images.size(), 1);
    QCOMPARE(images.first().attributes.value("width").toString(), QStringLiteral("8"));
    QCOMPARE(images.first().attributes.value("opacity").toString(), QStringLiteral("0.5"));
    const QString href = images.first().attributes.value("http://www.w3.org/1999/xlink", "href").toString();
    QVERIFY(href.startsWith(QStringLiteral("data:image/png;base64,")));
    const QImage decoded = QImage::fromData(QByteArray::fromBase64(href.mid(22).toLatin1()), "PNG");
    QCOMPARE(decoded.size(), QSize(4, 4));
    QCOMPARE(decoded.pixelColor(1, 1), QColor(Qt::red));
}

//=============================================================================================================

void TestDispSvgWriter::writesFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString fileName = dir.filePath(QStringLiteral("figure.svg"));

    SvgWriter svg;
    svg.setFileName(fileName.toStdString());
    QCOMPARE(svg.fileName(), fileName.toStdString());
    QVERIFY(!svg.outputDevice());
    svg.setSize(QSize(20, 20));
    {
        QPainter painter(&svg);
        painter.fillRect(QRect(0, 0, 20, 20), Qt::white);
    }

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    bool ok = false;
    const QList<SvgElement> elements = parse(file.readAll(), &ok);
    QVERIFY(ok);
    QCOMPARE(named(elements, "rect").size(), 1);
}

//=============================================================================================================

void TestDispSvgWriter::rendersWidget()
{
    QLabel label(QStringLiteral("Hello SVG"));
    label.resize(120, 40);

    QBuffer buffer;
    SvgWriter svg;
    svg.setOutputDevice(&buffer);
    svg.setSize(label.size());
    QPainter painter(&svg);
    label.render(&painter);
    QVERIFY(painter.end());

    bool ok = false;
    const QList<SvgElement> elements = parse(buffer.data(), &ok);
    QVERIFY(ok);
    QString written;
    for (const SvgElement& t : named(elements, "text")) {
        written += t.text;
    }
    QCOMPARE(written, QStringLiteral("Hello SVG"));
}

//=============================================================================================================

void TestDispSvgWriter::failsWithoutOutput()
{
    SvgWriter svg;
    svg.setSize(QSize(10, 10));
    QPainter painter;
    QVERIFY(!painter.begin(&svg));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestDispSvgWriter)
#include "test_disp_svg_writer.moc"
