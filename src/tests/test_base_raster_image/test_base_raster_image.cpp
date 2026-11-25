//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_raster_image.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests RasterImage: polygon coverage, text and PNG files that Qt decodes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/raster_image.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * @brief Tests RasterImage.
 */
class TestBaseRasterImage : public QObject
{
    Q_OBJECT

private slots:
    void pngDecodesToTheSamePixels();
    void alignedRectangleCoversWholePixels();
    void triangleCoverageMatchesItsArea();
    void overlappingPolygonUsesTheNonZeroRule();
    void textDrawsTheGlyphs();
    void translucentPolygonBlendsWithWhatIsUnderneath();
    void blendPixelMixesByAlpha();
    void overlappingPolygonsBlendOnce();
    void savePngReportsUnwritablePaths();
};

//=============================================================================================================

void TestBaseRasterImage::pngDecodesToTheSamePixels()
{
    RasterImage image(37, 11, {10, 20, 30});
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); x += 2) {
            image.set_pixel(x, y, {static_cast<std::uint8_t>(7 * x), static_cast<std::uint8_t>(23 * y), static_cast<std::uint8_t>(x * y)});
        }
    }
    const std::array<PointF, 3> triangle = {{{2.3, 1.1}, {30.7, 4.2}, {9.5, 10.6}}};
    image.fill_polygon(triangle, {250, 128, 0});

    const std::vector<std::uint8_t> png = image.encode_png();
    QImage decoded;
    QVERIFY(decoded.loadFromData(png.data(), static_cast<int>(png.size()), "PNG"));
    QCOMPARE(decoded.width(), image.width());
    QCOMPARE(decoded.height(), image.height());
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb qt = decoded.pixel(x, y);
            const Rgb own = image.pixel(x, y);
            QCOMPARE(qRed(qt), int(own.r));
            QCOMPARE(qGreen(qt), int(own.g));
            QCOMPARE(qBlue(qt), int(own.b));
        }
    }
}

//=============================================================================================================

void TestBaseRasterImage::translucentPolygonBlendsWithWhatIsUnderneath()
{
    RasterImage image(8, 8, {0, 0, 200});
    const std::array<PointF, 4> rectangle = {{{2, 2}, {6, 2}, {6, 6}, {2, 6}}};
    image.fill_polygon(rectangle, {255, 0, 0}, 128);
    QCOMPARE(image.pixel(3, 3), (Rgb{128, 0, 100}));
    QCOMPARE(image.pixel(1, 1), (Rgb{0, 0, 200}));

    // Opacity 255 is the opaque fill, 0 leaves the image alone
    image.fill_polygon(rectangle, {0, 255, 0}, 0);
    QCOMPARE(image.pixel(3, 3), (Rgb{128, 0, 100}));
    image.fill_polygon(rectangle, {0, 255, 0}, 255);
    QCOMPARE(image.pixel(3, 3), (Rgb{0, 255, 0}));
}

//=============================================================================================================

void TestBaseRasterImage::blendPixelMixesByAlpha()
{
    RasterImage image(2, 1, {100, 100, 100});
    image.blend_pixel(0, 0, {200, 0, 255}, 64);
    QCOMPARE(image.pixel(0, 0), (Rgb{125, 75, 139}));
    image.blend_pixel(1, 0, {1, 2, 3}, 255);
    QCOMPARE(image.pixel(1, 0), (Rgb{1, 2, 3}));
    // Outside the image and alpha 0 change nothing
    image.blend_pixel(5, 0, {1, 2, 3}, 255);
    image.blend_pixel(0, 0, {0, 0, 0}, 0);
    QCOMPARE(image.pixel(0, 0), (Rgb{125, 75, 139}));
}

//=============================================================================================================

void TestBaseRasterImage::overlappingPolygonsBlendOnce()
{
    RasterImage image(10, 4, {0, 0, 0});
    const std::vector<std::vector<PointF>> polygons = {{{0, 0}, {6, 0}, {6, 4}, {0, 4}}, {{4, 0}, {10, 0}, {10, 4}, {4, 4}}};
    image.fill_polygons(polygons, {200, 100, 0}, 128);
    QCOMPARE(image.pixel(1, 1), (Rgb{100, 50, 0}));
    QCOMPARE(image.pixel(5, 1), (Rgb{100, 50, 0}));
    QCOMPARE(image.pixel(8, 1), (Rgb{100, 50, 0}));
}

//=============================================================================================================

void TestBaseRasterImage::alignedRectangleCoversWholePixels()
{
    RasterImage image(8, 8);
    const std::array<PointF, 4> rectangle = {{{2, 3}, {6, 3}, {6, 5}, {2, 5}}};
    image.fill_polygon(rectangle, {255, 255, 255});
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const bool inside = x >= 2 && x < 6 && y >= 3 && y < 5;
            QCOMPARE(image.pixel(x, y), (inside ? Rgb{255, 255, 255} : Rgb{}));
        }
    }
}

//=============================================================================================================

void TestBaseRasterImage::triangleCoverageMatchesItsArea()
{
    RasterImage image(64, 64);
    const std::array<PointF, 3> triangle = {{{3.2, 5.7}, {60.1, 12.4}, {20.3, 58.9}}};
    image.fill_polygon(triangle, {255, 255, 255});

    double covered = 0.0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            covered += image.pixel(x, y).r / 255.0;
        }
    }
    const double area = std::abs((triangle[1].x - triangle[0].x) * (triangle[2].y - triangle[0].y) - (triangle[2].x - triangle[0].x) * (triangle[1].y - triangle[0].y)) / 2;
    QVERIFY2(std::abs(covered - area) < 0.005 * area, qPrintable(QString("%1 vs %2").arg(covered).arg(area)));
    QCOMPARE(image.pixel(25, 25), (Rgb{255, 255, 255}));
    QCOMPARE(image.pixel(60, 60), Rgb{});
}

//=============================================================================================================

void TestBaseRasterImage::overlappingPolygonUsesTheNonZeroRule()
{
    // A pentagram: its centre is enclosed twice and stays filled with the non-zero rule
    std::vector<PointF> star;
    for (int i = 0; i < 5; ++i) {
        const double angle = -std::numbers::pi / 2 + i * 4 * std::numbers::pi / 5;
        star.push_back({50 + 40 * std::cos(angle), 50 + 40 * std::sin(angle)});
    }
    RasterImage image(100, 100);
    image.fill_polygon(star, {0, 200, 0});
    QCOMPARE(image.pixel(50, 50), (Rgb{0, 200, 0}));
    QCOMPARE(image.pixel(50, 15), (Rgb{0, 200, 0}));
    QCOMPARE(image.pixel(5, 5), Rgb{});
}

//=============================================================================================================

void TestBaseRasterImage::textDrawsTheGlyphs()
{
    RasterImage image(20, 20);
    image.draw_text(1, 8, "1-", {255, 0, 0});
    // "1": stem in column 2 of the glyph, foot from column 1 to 3 in the last row
    QCOMPARE(image.pixel(3, 1), (Rgb{255, 0, 0}));
    QCOMPARE(image.pixel(2, 7), (Rgb{255, 0, 0}));
    QCOMPARE(image.pixel(4, 7), (Rgb{255, 0, 0}));
    QCOMPARE(image.pixel(1, 4), Rgb{});
    // "-": the middle row of the second glyph, six pixels to the right
    for (int x = 7; x < 12; ++x) {
        QCOMPARE(image.pixel(x, 4), (Rgb{255, 0, 0}));
    }
    QCOMPARE(image.pixel(7, 3), Rgb{});

    RasterImage scaled(20, 20);
    scaled.draw_text(0, 14, "-", {1, 2, 3}, 2);
    QCOMPARE(scaled.pixel(0, 6), (Rgb{1, 2, 3}));
    QCOMPARE(scaled.pixel(9, 7), (Rgb{1, 2, 3}));
    QCOMPARE(scaled.pixel(0, 5), Rgb{});
}

//=============================================================================================================

void TestBaseRasterImage::savePngReportsUnwritablePaths()
{
    QTemporaryDir dir;
    const RasterImage image(4, 4, {1, 2, 3});
    const std::filesystem::path path = dir.filePath("frame.png").toStdU16String();
    QVERIFY(image.save_png(path));
    QCOMPARE(QImage(dir.filePath("frame.png")).pixel(3, 3), qRgb(1, 2, 3));
    QCOMPARE(image.save_png(dir.filePath("missing/frame.png").toStdU16String()).error().code, ErrorCode::Io);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseRasterImage)
#include "test_base_raster_image.moc"
