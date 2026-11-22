//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_byte_stream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the endian helpers and byte streams, against QDataStream as the reference.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/byte_stream.h>
#include <mne/base/endian.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>
#include <QDataStream>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================

namespace
{

constexpr int kBenchValues = 1 << 20;

/** What QDataStream writes for @p values in FIFF's byte order, the reference for every big-endian check. */
QByteArray qtBigEndian(const std::vector<std::int32_t>& ints, const std::vector<float>& floats, const std::vector<double>& doubles)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for (const std::int32_t v : ints) {
        stream << v;
    }
    for (const float v : floats) {
        stream << v;
    }
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for (const double v : doubles) {
        stream << v;
    }
    return bytes;
}

std::vector<std::byte> toBytes(const QByteArray& bytes)
{
    std::vector<std::byte> out(static_cast<std::size_t>(bytes.size()));
    std::memcpy(out.data(), bytes.constData(), out.size());
    return out;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestUtilsByteStream
 *
 * @brief Tests ByteSwap helpers, MemoryByteStream and FileByteStream.
 */
class TestUtilsByteStream : public QObject
{
    Q_OBJECT

private slots:
    void byteSwapReversesBytes();
    void bigEndianMatchesQDataStream();
    void readsWhatQDataStreamWrote();
    void arraysSwapInPlace();
    void memoryStreamSeeksAndReportsShortReads();
    void memoryStreamOverwritesAndGrows();
    void fileStreamRoundTripAndModes();
    void fileStreamFailsOnMissingFile();
    void fileStreamCloseWritesAndEnds();
    void fileStreamResizesTheFileItHasOpen();
    void readFileBytesPlainAndGzip();
    void textFilesRoundTripExactly();
    void benchmarkReadBigEndianFloats_data();
    void benchmarkReadBigEndianFloats();
};

//=============================================================================================================

void TestUtilsByteStream::byteSwapReversesBytes()
{
    static_assert(byte_swap(std::uint16_t{0x1234}) == 0x3412);
    static_assert(byte_swap(std::uint32_t{0x12345678}) == 0x78563412);
    static_assert(byte_swap(std::uint64_t{0x0102030405060708}) == 0x0807060504030201);
    static_assert(byte_swap(std::int8_t{-5}) == -5);
    QCOMPARE(byte_swap(std::int16_t{-2}), std::int16_t{-257});
    QCOMPARE(byte_swap(byte_swap(-123456789)), -123456789);
    QCOMPARE(byte_swap(byte_swap(3.25f)), 3.25f);
    QCOMPARE(byte_swap(byte_swap(-1.0e300)), -1.0e300);
    QVERIFY(std::isnan(byte_swap(byte_swap(std::numeric_limits<double>::quiet_NaN()))));

    QCOMPARE(from_big_endian(to_big_endian(std::int64_t{-42})), std::int64_t{-42});
    std::uint32_t big = to_big_endian(std::uint32_t{0x01020304});
    unsigned char raw[4];
    std::memcpy(raw, &big, 4);
    QCOMPARE(raw[0], static_cast<unsigned char>(0x01));
    QCOMPARE(raw[3], static_cast<unsigned char>(0x04));
}

//=============================================================================================================

void TestUtilsByteStream::bigEndianMatchesQDataStream()
{
    const std::vector<std::int32_t> ints = {0, 1, -1, 101, std::numeric_limits<std::int32_t>::min()};
    const std::vector<float> floats = {0.0f, -2.5f, 1.0e-30f, std::numeric_limits<float>::infinity()};
    const std::vector<double> doubles = {3.141592653589793, -0.0, 1.0e300};

    MemoryByteStream stream;
    for (const std::int32_t v : ints) {
        QVERIFY(stream.write_big_endian(v));
    }
    QVERIFY(stream.write_big_endian(floats.data(), floats.size()));
    QVERIFY(stream.write_big_endian(doubles.data(), doubles.size()));

    QCOMPARE(stream.data(), toBytes(qtBigEndian(ints, floats, doubles)));
    QCOMPARE(stream.size(), std::uint64_t(5 * 4 + 4 * 4 + 3 * 8));
}

//=============================================================================================================

void TestUtilsByteStream::readsWhatQDataStreamWrote()
{
    const std::vector<std::int32_t> ints = {7, -300000};
    const std::vector<float> floats = {1.5f, -0.125f};
    const std::vector<double> doubles = {2.0e-9};
    MemoryByteStream stream(toBytes(qtBigEndian(ints, floats, doubles)));

    std::int32_t i0 = 0;
    std::int32_t i1 = 0;
    QVERIFY(stream.read_big_endian(i0) && stream.read_big_endian(i1));
    QCOMPARE(i0, 7);
    QCOMPARE(i1, -300000);
    float f[2] = {};
    QVERIFY(stream.read_big_endian(f, 2));
    QCOMPARE(f[0], 1.5f);
    QCOMPARE(f[1], -0.125f);
    double d = 0.0;
    QVERIFY(stream.read_big_endian(d));
    QCOMPARE(d, 2.0e-9);
    QVERIFY(stream.at_end());
    QVERIFY(!stream.read_big_endian(d));
}

//=============================================================================================================

void TestUtilsByteStream::arraysSwapInPlace()
{
    std::vector<std::int16_t> values = {0x0102, 0x0304, -1};
    byte_swap_in_place(values.data(), values.size());
    QCOMPARE(values[0], std::int16_t{0x0201});
    QCOMPARE(values[1], std::int16_t{0x0403});
    QCOMPARE(values[2], std::int16_t{-1});
    byte_swap_in_place(values.data(), 0);
    QCOMPARE(values[0], std::int16_t{0x0201});
}

//=============================================================================================================

void TestUtilsByteStream::memoryStreamSeeksAndReportsShortReads()
{
    MemoryByteStream stream({std::byte{1}, std::byte{2}, std::byte{3}});
    QCOMPARE(stream.pos(), std::uint64_t(0));
    QVERIFY(stream.seek(2));
    std::byte buffer[4] = {};
    QCOMPARE(stream.read(buffer, 4), std::size_t(1));
    QCOMPARE(buffer[0], std::byte{3});
    QVERIFY(stream.at_end());
    QVERIFY(stream.seek(3));
    QVERIFY(!stream.seek(4));
    QCOMPARE(stream.pos(), std::uint64_t(3));

    std::int32_t value = 0;
    QVERIFY(stream.seek(0));
    QVERIFY(!stream.read_big_endian(value)); // three bytes only
}

//=============================================================================================================

void TestUtilsByteStream::memoryStreamOverwritesAndGrows()
{
    MemoryByteStream stream;
    QVERIFY(stream.write_big_endian(std::int32_t{1}));
    QVERIFY(stream.write_big_endian(std::int32_t{2}));
    QVERIFY(stream.seek(0));
    QVERIFY(stream.write_big_endian(std::int32_t{9}));
    QCOMPARE(stream.size(), std::uint64_t(8));
    QVERIFY(stream.seek(6));
    QVERIFY(stream.write_big_endian(std::int32_t{0x0A0B0C0D}));
    QCOMPARE(stream.size(), std::uint64_t(10));

    QVERIFY(stream.seek(0));
    std::int32_t first = 0;
    QVERIFY(stream.read_big_endian(first));
    QCOMPARE(first, 9);
    QCOMPARE(stream.data()[6], std::byte{0x0A});
    QCOMPARE(stream.data()[9], std::byte{0x0D});
}

//=============================================================================================================

void TestUtilsByteStream::fileStreamRoundTripAndModes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::filesystem::path path = dir.filePath(QStringLiteral("stream.bin")).toStdString();
    const std::vector<double> values = {1.0, -2.0, 4.5};
    {
        auto stream = FileByteStream::open(path, FileByteStream::Mode::Write);
        QVERIFY(stream);
        QVERIFY(stream->write_big_endian(std::int32_t{3}));
        QVERIFY(stream->write_big_endian(values.data(), values.size()));
        QCOMPARE(stream->size(), std::uint64_t(4 + 3 * 8));
    }
    {
        // Updating a field in place keeps the rest of the file, as FIFF does for its directory pointer.
        auto stream = FileByteStream::open(path, FileByteStream::Mode::ReadWrite);
        QVERIFY(stream);
        QVERIFY(stream->seek(0));
        QVERIFY(stream->write_big_endian(std::int32_t{-3}));
        QCOMPARE(stream->size(), std::uint64_t(28));
    }
    QFile file(QString::fromStdString(path.string()));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QDataStream reference(&file);
    reference.setByteOrder(QDataStream::BigEndian);
    reference.setFloatingPointPrecision(QDataStream::DoublePrecision);
    qint32 count = 0;
    double third = 0.0;
    reference >> count;
    reference.skipRawData(16);
    reference >> third;
    QCOMPARE(count, -3);
    QCOMPARE(third, 4.5);

    auto stream = FileByteStream::open(path, FileByteStream::Mode::Read);
    QVERIFY(stream);
    QVERIFY(stream->seek(4));
    std::vector<double> back(3);
    QVERIFY(stream->read_big_endian(back.data(), back.size()));
    QCOMPARE(back, values);
    QVERIFY(stream->at_end());
    QCOMPARE(stream->write("x", 1), std::size_t(0)); // read-only
    QVERIFY(!stream->resize(4));
    stream.reset();

    // Cutting a file short drops its tail, as FIFF does when it discards a directory before appending.
    auto update = FileByteStream::open(path, FileByteStream::Mode::ReadWrite);
    QVERIFY(update);
    QVERIFY(update->seek(20));
    QVERIFY(update->resize(12));
    QCOMPARE(update->size(), std::uint64_t(12));
    QCOMPARE(update->pos(), std::uint64_t(12));
    QVERIFY(update->write_big_endian(4.5));
    update.reset();
    QCOMPARE(std::filesystem::file_size(path), std::uintmax_t(20));
    auto reread = FileByteStream::open(path, FileByteStream::Mode::Read);
    QVERIFY(reread && reread->seek(4));
    QVERIFY(reread->read_big_endian(back.data(), 2));
    QCOMPARE(back[0], 1.0);
    QCOMPARE(back[1], 4.5);
}

//=============================================================================================================

void TestUtilsByteStream::fileStreamFailsOnMissingFile()
{
    QTemporaryDir dir;
    QVERIFY(!FileByteStream::open(dir.filePath(QStringLiteral("missing.bin")).toStdString(), FileByteStream::Mode::Read));
    QVERIFY(!FileByteStream::open(dir.filePath(QStringLiteral("missing.bin")).toStdString(), FileByteStream::Mode::ReadWrite));
    QVERIFY(!FileByteStream::open(dir.filePath(QStringLiteral("no/dir/out.bin")).toStdString(), FileByteStream::Mode::Write));
}

//=============================================================================================================

void TestUtilsByteStream::fileStreamCloseWritesAndEnds()
{
    QTemporaryDir dir;
    const std::filesystem::path path = dir.filePath(QStringLiteral("closed.bin")).toStdString();
    auto stream = FileByteStream::open(path, FileByteStream::Mode::Write);
    QVERIFY(stream);
    QVERIFY(stream->is_open());
    QVERIFY(stream->write_big_endian(std::int32_t{7}));
    stream->close();
    QVERIFY(!stream->is_open());
    // The bytes are on disk while the stream object still exists, and later calls fail instead of crashing.
    QCOMPARE(std::filesystem::file_size(path), std::uintmax_t(4));
    QCOMPARE(stream->write("x", 1), std::size_t(0));
    char byte = 0;
    QCOMPARE(stream->read(&byte, 1), std::size_t(0));
    QVERIFY(!stream->seek(0));
    stream->close();

    // Reopening reads what was written, without truncating it
    QVERIFY(stream->reopen());
    QVERIFY(stream->is_open());
    std::int32_t value = 0;
    QVERIFY(stream->read_big_endian(value));
    QCOMPARE(value, std::int32_t{7});
    QCOMPARE(std::filesystem::file_size(path), std::uintmax_t(4));
    QVERIFY(stream->reopen());
    MemoryByteStream memory;
    memory.close();
    QVERIFY(memory.reopen());
}

//=============================================================================================================

void TestUtilsByteStream::fileStreamResizesTheFileItHasOpen()
{
    // Cutting a file goes through the open file, not its path: Windows refuses to resize a file through its path
    // while the stream holds it, and the path may by now name another file or none.
    QTemporaryDir dir;
    const std::filesystem::path path = path_from_utf8(dir.filePath(QStringLiteral("open.bin")).toStdString());
    const std::filesystem::path moved = path_from_utf8(dir.filePath(QStringLiteral("moved.bin")).toStdString());
    auto stream = FileByteStream::open(path, FileByteStream::Mode::Write);
    QVERIFY(stream);
    QVERIFY(stream->write_big_endian(std::int64_t{1}) && stream->write_big_endian(std::int64_t{2}));
    QVERIFY(stream->flush());
    std::error_code error;
    std::filesystem::rename(path, moved, error);
    if (error) {
        QSKIP("this file system does not rename an open file");
    }
    QVERIFY(stream->resize(8));
    QCOMPARE(stream->size(), std::uint64_t(8));
    stream.reset();
    QCOMPARE(std::filesystem::file_size(moved), std::uintmax_t(8));
}

//=============================================================================================================

void TestUtilsByteStream::readFileBytesPlainAndGzip()
{
    QTemporaryDir dir;
    const auto write = [&dir](const char* name, std::initializer_list<unsigned char> bytes) {
        const std::filesystem::path path = dir.filePath(QString::fromLatin1(name)).toStdString();
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(std::data(bytes)), static_cast<std::streamsize>(bytes.size()));
        return path;
    };
    // gzip -n of "hello"
    const std::filesystem::path gz = write("hello.gz", {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x86, 0xa6, 0x10, 0x36, 0x05, 0x00, 0x00, 0x00});
    const std::filesystem::path cut = write("cut.gz", {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xcb, 0x48, 0xcd});
    const std::filesystem::path plain = write("plain.bin", {'h', 'i'});
    const std::filesystem::path empty = write("empty.bin", {});

    const auto text = [](const std::vector<std::byte>& bytes) {
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    };
    QCOMPARE(text(read_file_bytes(gz, Compression::Gzip).value()), std::string("hello"));
    QCOMPARE(text(read_file_bytes(plain).value()), std::string("hi"));
    QVERIFY(read_file_bytes(empty).value().empty());

    const auto truncated = read_file_bytes(cut, Compression::Gzip);
    QVERIFY(!truncated && truncated.error().code == ErrorCode::Format);
    const auto notGzip = read_file_bytes(plain, Compression::Gzip);
    QVERIFY(!notGzip && notGzip.error().code == ErrorCode::Format);
    const auto missing = read_file_bytes(dir.filePath(QStringLiteral("missing.bin")).toStdString());
    QVERIFY(!missing && missing.error().code == ErrorCode::Io);
}

//=============================================================================================================

void TestUtilsByteStream::textFilesRoundTripExactly()
{
    QTemporaryDir dir;
    const std::filesystem::path path = path_from_utf8(dir.filePath(QStringLiteral("text.json")).toStdString());
    // Bytes are written as given: no CR is added, and NUL and UTF-8 survive.
    const std::string text("{\n  \"Name\": \"\xc3\xa4\"\n}\n\0end", 23);
    QVERIFY(write_text_file(path, text).has_value());
    QCOMPARE(read_text_file(path).value(), text);
    QVERIFY(write_text_file(path, "short").has_value());
    QCOMPARE(read_text_file(path).value(), std::string("short"));

    const auto missing = read_text_file(path.parent_path() / "missing.txt");
    QVERIFY(!missing && missing.error().code == ErrorCode::Io);
    const auto unwritable = write_text_file(path.parent_path() / "no" / "dir.txt", text);
    QVERIFY(!unwritable && unwritable.error().code == ErrorCode::Io);
    std::vector<std::byte> bytes;
    bytes.reserve(256);
    for (int i = 0; i < 256; ++i) {
        bytes.push_back(static_cast<std::byte>(i));
    }
    QVERIFY(write_file_bytes(path, bytes).has_value());
    QCOMPARE(read_file_bytes(path).value(), bytes);
    QVERIFY(write_file_bytes(path, std::span(bytes).first(3)).has_value());
    QCOMPARE(read_file_bytes(path).value().size(), std::size_t(3));
    const auto unwritableBytes = write_file_bytes(path.parent_path() / "no" / "dir.bin", bytes);
    QVERIFY(!unwritableBytes && unwritableBytes.error().code == ErrorCode::Io);
}

//=============================================================================================================

void TestUtilsByteStream::benchmarkReadBigEndianFloats_data()
{
    QTest::addColumn<bool>("qt");
    QTest::addColumn<bool>("file");
    QTest::newRow("QDataStream on QByteArray") << true << false;
    QTest::newRow("MemoryByteStream") << false << false;
    QTest::newRow("QDataStream on QFile") << true << true;
    QTest::newRow("FileByteStream") << false << true;
}

//=============================================================================================================

void TestUtilsByteStream::benchmarkReadBigEndianFloats()
{
    QFETCH(bool, qt);
    QFETCH(bool, file);
    std::vector<float> floats(kBenchValues);
    for (int i = 0; i < kBenchValues; ++i) {
        floats[static_cast<std::size_t>(i)] = static_cast<float>(i) * 0.5f;
    }
    const QByteArray bytes = qtBigEndian({}, floats, {});
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("floats.bin"));
    if (file) {
        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly) && out.write(bytes) == bytes.size());
    }
    std::vector<float> back(floats.size());
    if (qt) {
        QBENCHMARK
        {
            QFile in(path);
            QBuffer buffer;
            buffer.setData(bytes);
            QIODevice& device = file ? static_cast<QIODevice&>(in) : buffer;
            device.open(QIODevice::ReadOnly);
            QDataStream stream(&device);
            stream.setByteOrder(QDataStream::BigEndian);
            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            for (float& v : back) {
                stream >> v;
            }
        }
    } else if (file) {
        QBENCHMARK
        {
            auto stream = FileByteStream::open(path.toStdString(), FileByteStream::Mode::Read);
            stream->read_big_endian(back.data(), back.size());
        }
    } else {
        const std::vector<std::byte> raw = toBytes(bytes);
        QBENCHMARK
        {
            MemoryByteStream stream(raw);
            stream.read_big_endian(back.data(), back.size());
        }
    }
    QCOMPARE(back, floats);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsByteStream)
#include "test_base_byte_stream.moc"
