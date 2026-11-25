//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     golden.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Compares rendered SVG documents and images with golden files committed next to a test.
 *
 * An SVG must match its golden file byte for byte. An image may differ by a few levels in a few pixels, which
 * floating-point differences between platforms cause at anti-aliased edges. With the environment variable
 * MNE_UPDATE_GOLDEN=1 the comparisons write the current output as the new golden files instead:
 *
 * @code
 * QVERIFY2(golden::matches_svg(mne::scene::to_svg(list), GOLDEN_DIR "/plot.svg"), golden::last_message());
 * @endcode
 */

#ifndef MNE_TESTFRAMES_GOLDEN_H
#define MNE_TESTFRAMES_GOLDEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/raster_image.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace golden
{

/** @return Whether the golden files are rewritten instead of compared (MNE_UPDATE_GOLDEN=1). */
inline bool updating()
{
    const char* value = std::getenv("MNE_UPDATE_GOLDEN");
    return value && std::strcmp(value, "1") == 0;
}

/** @return The explanation of the last failed comparison. */
inline QByteArray& last_message()
{
    static QByteArray message;
    return message;
}

inline bool write_file(const QString& path, const QByteArray& data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(data) != data.size()) {
        last_message() = "cannot write " + path.toUtf8();
        return false;
    }
    return true;
}

/**
 * @param[in] svg    Rendered document.
 * @param[in] path   Golden file.
 *
 * @return Whether @p svg equals the golden file (always true while updating).
 */
inline bool matches_svg(const std::string& svg, const QString& path)
{
    const QByteArray actual = QByteArray::fromStdString(svg);
    if (updating()) {
        return write_file(path, actual);
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        last_message() = "missing golden file " + path.toUtf8() + " (run with MNE_UPDATE_GOLDEN=1)";
        return false;
    }
    const QByteArray expected = file.readAll();
    if (expected == actual) {
        return true;
    }
    qsizetype at = 0;
    while (at < expected.size() && at < actual.size() && expected[at] == actual[at]) {
        ++at;
    }
    const qsizetype from = std::max<qsizetype>(0, at - 60);
    last_message() = path.toUtf8() + " differs at byte " + QByteArray::number(at) + "\n expected: " + expected.mid(from, 160) +
        "\n actual:   " + actual.mid(from, 160);
    return false;
}

/**
 * @param[in] image       Rendered image.
 * @param[in] path        Golden PNG file.
 * @param[in] tolerance   Largest allowed difference of a colour channel.
 * @param[in] fraction    Largest allowed fraction of pixels beyond the tolerance.
 *
 * @return Whether @p image matches the golden file (always true while updating).
 */
inline bool matches_png(const mne::RasterImage& image, const QString& path, int tolerance = 8, double fraction = 0.002)
{
    if (updating()) {
        const std::vector<std::uint8_t> png = image.encode_png();
        return write_file(path, QByteArray(reinterpret_cast<const char*>(png.data()), static_cast<qsizetype>(png.size())));
    }
    QImage expected;
    if (!expected.load(path, "PNG")) {
        last_message() = "missing golden file " + path.toUtf8() + " (run with MNE_UPDATE_GOLDEN=1)";
        return false;
    }
    if (expected.width() != image.width() || expected.height() != image.height()) {
        last_message() = path.toUtf8() + ": size " + QByteArray::number(image.width()) + "x" + QByteArray::number(image.height()) +
            ", golden " + QByteArray::number(expected.width()) + "x" + QByteArray::number(expected.height());
        return false;
    }
    long long beyond = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb e = expected.pixel(x, y);
            const mne::Rgb a = image.pixel(x, y);
            const int diff = std::max({std::abs(qRed(e) - a.r), std::abs(qGreen(e) - a.g), std::abs(qBlue(e) - a.b)});
            beyond += diff > tolerance ? 1 : 0;
        }
    }
    const long long allowed = static_cast<long long>(fraction * image.width() * image.height());
    if (beyond > allowed) {
        last_message() = path.toUtf8() + ": " + QByteArray::number(beyond) + " pixels differ by more than " + QByteArray::number(tolerance) +
            " (allowed " + QByteArray::number(allowed) + ")";
        return false;
    }
    return true;
}

} // namespace golden

#endif // MNE_TESTFRAMES_GOLDEN_H
