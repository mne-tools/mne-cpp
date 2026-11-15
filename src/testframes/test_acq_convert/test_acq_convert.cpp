//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_convert.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests for decode(), encode(), decodeBits(), encodeBits(), decodePart() and AcqReader::read().
 *
 * Every converter result is compared bit for bit with a scalar reference written independently of the
 * library, for every sample format, both ring layouts, views of one and two parts, row- and column-major
 * destinations, channel subsets and column offsets. The target is built with -ffp-contract=off, like the
 * library, so neither side fuses a multiply and an add.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_convert.h>
#include <acq/acq_stream.h>

#include <array>
#include <bit>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

constexpr std::array<AcqSampleFormat, 7> kFormats = {AcqSampleFormat::Int16, AcqSampleFormat::Int24Packed,
                                                     AcqSampleFormat::Int32, AcqSampleFormat::Float32,
                                                     AcqSampleFormat::Float64, AcqSampleFormat::ComplexInt16,
                                                     AcqSampleFormat::ComplexFloat32};

bool isInteger(AcqSampleFormat f)
{
    return f == AcqSampleFormat::Int16 || f == AcqSampleFormat::Int24Packed || f == AcqSampleFormat::Int32 ||
        f == AcqSampleFormat::ComplexInt16;
}

/** Bits of one integer word (one component for complex int16). */
int wordBits(AcqSampleFormat f)
{
    switch (f) {
        case AcqSampleFormat::Int24Packed:
            return 24;
        case AcqSampleFormat::Int32:
            return 32;
        default:
            return 16;
    }
}

std::size_t componentBytes(AcqSampleFormat f)
{
    switch (f) {
        case AcqSampleFormat::Int24Packed:
            return 3;
        case AcqSampleFormat::Int32:
        case AcqSampleFormat::Float32:
        case AcqSampleFormat::ComplexFloat32:
            return 4;
        case AcqSampleFormat::Float64:
            return 8;
        default:
            return 2;
    }
}

/** Deterministic test data. */
struct Lcg
{
    std::uint64_t state;

    std::uint64_t next()
    {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state >> 11U;
    }

    double uniform(double lo, double hi)
    {
        return lo + (hi - lo) * static_cast<double>(next() & ((1ULL << 52U) - 1)) / static_cast<double>(1ULL << 52U);
    }
};

//=============================================================================================================
// SCALAR REFERENCE (independent of the library)
//=============================================================================================================

std::uint32_t readWord(const std::byte* p, std::size_t bytes)
{
    std::uint32_t w = 0;
    for (std::size_t i = 0; i < bytes; ++i) {
        w |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[i])) << (8U * i);
    }
    return w;
}

void writeWord(std::byte* p, std::size_t bytes, std::uint32_t w)
{
    for (std::size_t i = 0; i < bytes; ++i) {
        p[i] = static_cast<std::byte>((w >> (8U * i)) & 0xFFU);
    }
}

/** Two's complement value of the low @p bits of @p w. */
std::int64_t signedLow(std::uint32_t w, int bits)
{
    const std::uint64_t low = w & ((bits == 32 ? 0xFFFFFFFFULL : (1ULL << bits) - 1));
    return low >= (1ULL << (bits - 1)) ? static_cast<std::int64_t>(low) - (std::int64_t(1) << bits)
                                       : static_cast<std::int64_t>(low);
}

/** Raw integer of component @p comp, with the analog value restored to the full word. */
std::int64_t refInteger(const std::byte* element, AcqSampleFormat f, int comp, int digitalBits)
{
    const std::size_t cb = componentBytes(f);
    const std::uint32_t word = readWord(element + comp * cb, cb);
    const int bits = wordBits(f);
    const int d = comp == 0 ? digitalBits : 0;
    return signedLow(word, bits - d) * (std::int64_t(1) << d);
}

double refFloating(const std::byte* element, AcqSampleFormat f, int comp)
{
    if (f == AcqSampleFormat::Float64) {
        double v = 0;
        std::memcpy(&v, element, sizeof(v));
        return v;
    }
    float v = 0;
    std::memcpy(&v, element + comp * sizeof(float), sizeof(v));
    return v;
}

template<typename Real>
Real refComponent(const std::byte* element, AcqSampleFormat f, int comp, int digitalBits, bool physical,
                  double scale, double offset)
{
    Real v = isInteger(f) ? static_cast<Real>(refInteger(element, f, comp, digitalBits))
                          : static_cast<Real>(refFloating(element, f, comp));
    if (physical) {
        const Real scaled = v * static_cast<Real>(scale);
        v = comp == 0 ? scaled + static_cast<Real>(offset) : scaled;
    }
    return v;
}

template<typename T>
T refDecode(const std::byte* element, AcqSampleFormat f, int digitalBits, bool physical, double scale, double offset)
{
    if constexpr (std::is_same_v<T, std::complex<float>>) {
        const float re = refComponent<float>(element, f, 0, digitalBits, physical, scale, offset);
        const float im = isComplex(f) ? refComponent<float>(element, f, 1, digitalBits, physical, scale, offset) : 0.0F;
        return {re, im};
    } else {
        return refComponent<T>(element, f, 0, digitalBits, physical, scale, offset);
    }
}

/** Reference encoder of one real component into a native word; @return true if clipped. */
bool refEncodeComponent(double value, AcqSampleFormat f, int comp, int digitalBits, bool physical, double scale,
                        double offset, double rangeMin, double rangeMax, std::byte* element)
{
    bool clipped = false;
    if (std::isnan(value)) {
        value = 0.0;
        clipped = true;
        physical = false;
    }
    if (physical) {
        if (!std::isnan(rangeMin) && value < rangeMin) {
            value = rangeMin;
            clipped = true;
        }
        if (!std::isnan(rangeMax) && value > rangeMax) {
            value = rangeMax;
            clipped = true;
        }
        value = (value - (comp == 0 ? offset : 0.0)) / scale;
    }
    if (f == AcqSampleFormat::Float64) {
        if (std::isinf(value)) {
            value = std::copysign(std::numeric_limits<double>::max(), value);
            clipped = true;
        }
        std::memcpy(element, &value, sizeof(value));
        return clipped;
    }
    if (!isInteger(f)) {
        const double limit = std::numeric_limits<float>::max();
        if (value > limit || value < -limit) {
            value = std::copysign(limit, value);
            clipped = true;
        }
        const auto v = static_cast<float>(value);
        std::memcpy(element + comp * sizeof(float), &v, sizeof(v));
        return clipped;
    }
    const int bits = wordBits(f);
    const int d = comp == 0 ? digitalBits : 0;
    const int analogBits = bits - d;
    double q = std::nearbyint(std::ldexp(value, -d));
    const double lo = -std::ldexp(1.0, analogBits - 1);
    const double hi = std::ldexp(1.0, analogBits - 1) - 1.0;
    if (q < lo || q > hi) {
        q = q < lo ? lo : hi;
        clipped = true;
    }
    const std::size_t cb = componentBytes(f);
    const std::uint32_t old = readWord(element + comp * cb, cb);
    const std::uint64_t lowMask = analogBits == 32 ? 0xFFFFFFFFULL : (1ULL << analogBits) - 1;
    const auto low = static_cast<std::uint64_t>(static_cast<std::int64_t>(q)) & lowMask;
    writeWord(element + comp * cb, cb, static_cast<std::uint32_t>((old & ~lowMask) | low));
    return clipped;
}

//=============================================================================================================
// STREAM HELPERS
//=============================================================================================================

struct Ring
{
    AcqWriter writer;
    AcqReader reader;
};

AcqStreamInfo makeInfo(AcqSampleFormat format, AcqLayout layout, std::size_t channels, int digitalBits)
{
    AcqStreamInfo info;
    info.name = "convert";
    info.format = format;
    info.layout = layout;
    info.sfreq = 1000.0;
    for (std::size_t c = 0; c < channels; ++c) {
        AcqChannelInfo ch;
        ch.name = "ch" + std::to_string(c);
        ch.scale = 0.5 + 0.37 * static_cast<double>(c);
        ch.offset = c % 2 == 0 ? 0.0 : -3.25 * static_cast<double>(c);
        ch.digitalBits = isInteger(format) && c % 3 == 1 ? static_cast<std::uint8_t>(digitalBits) : 0;
        info.channels.push_back(ch);
    }
    return info;
}

Ring makeRing(const AcqStreamInfo& info, std::size_t capacity, bool mirror)
{
    AcqStreamOptions o;
    o.capacitySamples = capacity;
    o.mirror = mirror;
    o.lockMemory = false;
    auto writer = AcqWriter::create(info, o);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    return {std::move(*writer), std::move(*reader)};
}

/** Fills the next @p samples samples with random native bytes and commits them. */
void produceRandom(AcqWriter& writer, std::size_t samples, Lcg& rng)
{
    auto view = writer.beginWrite(samples);
    QVERIFY(view);
    const AcqSampleFormat f = view->format();
    for (std::size_t p = 0; p < view->partCount(); ++p) {
        const AcqWritePart& part = view->part(p);
        for (std::size_t s = 0; s < part.samples; ++s) {
            for (std::size_t c = 0; c < view->channelCount(); ++c) {
                std::byte* e = part.at(c, s);
                const int comps = isComplex(f) ? 2 : 1;
                for (int k = 0; k < comps; ++k) {
                    if (isInteger(f)) {
                        writeWord(e + k * componentBytes(f), componentBytes(f), static_cast<std::uint32_t>(rng.next()));
                    } else if (f == AcqSampleFormat::Float64) {
                        const double v = rng.uniform(-1e5, 1e5);
                        std::memcpy(e, &v, sizeof(v));
                    } else {
                        const auto v = static_cast<float>(rng.uniform(-1e5, 1e5));
                        std::memcpy(e + k * sizeof(float), &v, sizeof(v));
                    }
                }
            }
        }
    }
    writer.commit(samples);
}

const std::byte* elementOf(const AcqBlockView& view, std::size_t c, std::size_t s)
{
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        if (s < view.part(p).samples) {
            return view.part(p).at(c, s);
        }
        s -= view.part(p).samples;
    }
    return nullptr;
}

template<typename T>
bool sameBits(const T& a, const T& b)
{
    using Bytes = std::array<std::byte, sizeof(T)>;
    return std::bit_cast<Bytes>(a) == std::bit_cast<Bytes>(b);
}

/**
 * Decodes @p view into matrices of every orientation, channel subset and column offset and compares each
 * element with the reference; @return a description of the first mismatch, or an empty string.
 */
template<typename T>
std::string checkDecode(const AcqBlockView& view, const AcqStreamInfo& info)
{
    const std::size_t channels = view.channelCount();
    const std::size_t samples = view.sampleCount();
    for (const bool rowMajor : {false, true}) {
        for (const AcqScaling scaling : {AcqScaling::Raw, AcqScaling::Physical}) {
            for (const AcqChannelRange range : {AcqChannelRange{}, AcqChannelRange{1, channels - 2}}) {
                const std::size_t rows = range.count == std::numeric_limits<std::size_t>::max() ? channels : range.count;
                constexpr std::size_t kFirstColumn = 3;
                const std::size_t cols = kFirstColumn + samples + 2;
                std::vector<T> storage(rows * cols, T(-7));
                const AcqMatrixRef<T> out = rowMajor ? AcqMatrixRef<T>::rowMajor(storage.data(), rows, cols)
                                                     : AcqMatrixRef<T>::columnMajor(storage.data(), rows, cols);
                AcqConvertOptions options;
                options.scaling = scaling;
                options.channels = range;
                options.firstColumn = kFirstColumn;
                const AcqError error = decode(view, out, options);
                if (!error.ok()) {
                    return "decode failed: " + error.text();
                }
                for (std::size_t r = 0; r < rows; ++r) {
                    const std::size_t c = range.first + r;
                    for (std::size_t col = 0; col < cols; ++col) {
                        const T got = out.data[static_cast<std::ptrdiff_t>(r) * out.rowStride +
                                               static_cast<std::ptrdiff_t>(col) * out.colStride];
                        T expected = T(-7);
                        if (col >= kFirstColumn && col < kFirstColumn + samples) {
                            expected = refDecode<T>(elementOf(view, c, col - kFirstColumn), view.format(),
                                                    info.channels[c].digitalBits, scaling == AcqScaling::Physical,
                                                    info.channels[c].scale, info.channels[c].offset);
                        }
                        if (!sameBits(got, expected)) {
                            return "mismatch at channel " + std::to_string(c) + ", column " + std::to_string(col) +
                                (rowMajor ? " (row-major)" : " (column-major)") +
                                (scaling == AcqScaling::Raw ? " raw" : " physical");
                        }
                    }
                }
            }
        }
    }
    return {};
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestAcqConvert
 *
 * @brief Bit-exact tests of the sample converters against a scalar reference.
 */
class TestAcqConvert : public QObject
{
    Q_OBJECT

private slots:
    void testDecodeAllFormats_data();
    void testDecodeAllFormats();
    void testDecodeErrors();
    void testEncodeRoundingAndClipping();
    void testEncodeAllFormatsBitExact();
    void testEncodeComplexAndOffsets();
    void testDigitalBitsSpectrumPacking();
    void testDecodePart();
    void testReaderRead();
    void testDeviceMemoryRefused();
    void testFloatSaturationAndWideBitFields();
    void testWideStreams();
};

//=============================================================================================================

void TestAcqConvert::testDecodeAllFormats_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("layout");
    QTest::addColumn<bool>("mirror");
    QTest::addColumn<int>("digitalBits");
    for (const AcqSampleFormat f : kFormats) {
        for (const AcqLayout layout : {AcqLayout::Interleaved, AcqLayout::Planar}) {
            for (const bool mirror : {false, true}) {
                // Channels with digital bits take the general converter, channels without them the fast one.
                for (const int digitalBits : {0, 3}) {
                    const std::string name = std::string(sampleFormatName(f)) +
                        (layout == AcqLayout::Interleaved ? " interleaved" : " planar") + (mirror ? " mirrored" : "") +
                        (digitalBits > 0 ? " digital" : "");
                    QTest::newRow(name.c_str()) << static_cast<int>(f) << static_cast<int>(layout) << mirror << digitalBits;
                }
            }
        }
    }
}

void TestAcqConvert::testDecodeAllFormats()
{
    QFETCH(int, format);
    QFETCH(int, layout);
    QFETCH(bool, mirror);
    QFETCH(int, digitalBits);
    const AcqStreamInfo info =
        makeInfo(static_cast<AcqSampleFormat>(format), static_cast<AcqLayout>(layout), 7, digitalBits);
    // A ring of 4096 samples mirrors where the size allows; without the mirror the second block wraps in two parts.
    Ring ring = makeRing(info, mirror ? 4096 : 64, mirror);
    Lcg rng{static_cast<std::uint64_t>(format * 31 + layout)};
    const std::size_t first = mirror ? 4080 : 50; // the next 40 samples cross the end of the ring
    produceRandom(ring.writer, first, rng);
    auto skip = ring.reader.acquire(first, 0ns);
    QVERIFY(skip);
    QVERIFY(ring.reader.release(*skip).ok());
    produceRandom(ring.writer, 40, rng);
    auto view = ring.reader.acquire(40, 0ns);
    QVERIFY(view);
    QCOMPARE(view->partCount(), std::size_t(ring.writer.stream().stats().mirrored ? 1 : 2));

    std::string failure;
    if (!isComplex(view->format())) {
        failure = checkDecode<float>(*view, info);
        QVERIFY2(failure.empty(), failure.c_str());
        failure = checkDecode<double>(*view, info);
        QVERIFY2(failure.empty(), failure.c_str());
    }
    failure = checkDecode<std::complex<float>>(*view, info);
    QVERIFY2(failure.empty(), failure.c_str());
    QVERIFY(ring.reader.release(*view).ok());
}

//=============================================================================================================

void TestAcqConvert::testDecodeErrors()
{
    const AcqStreamInfo info = makeInfo(AcqSampleFormat::ComplexInt16, AcqLayout::Interleaved, 4, 0);
    Ring ring = makeRing(info, 64, false);
    Lcg rng{1};
    produceRandom(ring.writer, 10, rng);
    auto view = ring.reader.acquire(10, 0ns);
    QVERIFY(view);
    std::vector<float> f(4 * 10);
    QCOMPARE(decode(*view, AcqMatrixRef<float>::columnMajor(f.data(), 4, 10)).code(), AcqErrorCode::UnsupportedFormat);
    std::vector<std::complex<float>> z(4 * 10);
    QVERIFY(decode(*view, AcqMatrixRef<std::complex<float>>::columnMajor(z.data(), 4, 10)).ok());
    QCOMPARE(decode(*view, AcqMatrixRef<std::complex<float>>::columnMajor(z.data(), 3, 10)).code(),
             AcqErrorCode::ShapeMismatch);
    QCOMPARE(decode(*view, AcqMatrixRef<std::complex<float>>::columnMajor(z.data(), 4, 9)).code(),
             AcqErrorCode::ShapeMismatch);
    AcqConvertOptions offset;
    offset.firstColumn = 1;
    QCOMPARE(decode(*view, AcqMatrixRef<std::complex<float>>::columnMajor(z.data(), 4, 10), offset).code(),
             AcqErrorCode::ShapeMismatch);
    AcqConvertOptions outside;
    outside.channels = {3, 2};
    QCOMPARE(decode(*view, AcqMatrixRef<std::complex<float>>::columnMajor(z.data(), 4, 10), outside).code(),
             AcqErrorCode::ShapeMismatch);
    QCOMPARE(decode(*view, AcqMatrixRef<std::complex<float>>{}).code(), AcqErrorCode::ShapeMismatch);
    QVERIFY(ring.reader.release(*view).ok());
}

//=============================================================================================================

void TestAcqConvert::testEncodeRoundingAndClipping()
{
    // int16 with scale 1, range [-100, 100] V: ties to even, NaN and saturation are counted.
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 1, 0);
    info.channels[0].scale = 1.0;
    info.channels[0].offset = 0.0;
    info.channels[0].rangeMin = -100.0;
    info.channels[0].rangeMax = 100.0;
    Ring ring = makeRing(info, 64, false);
    const std::vector<double> in = {2.5, 3.5, -2.5, -3.5, 0.49999999999999994, 99.6, 150.0, -1e9,
                                    std::numeric_limits<double>::quiet_NaN(), -0.0};
    const std::vector<std::int16_t> expected = {2, 4, -2, -4, 0, 100, 100, -100, 0, 0};
    auto view = ring.writer.beginWrite(in.size());
    QVERIFY(view);
    auto result = encode(AcqMatrixRef<const double>::rowMajor(in.data(), 1, in.size()), *view);
    QVERIFY(result);
    QCOMPARE(result->clipped, std::uint64_t(3));
    ring.writer.commit(in.size());
    auto back = ring.reader.acquire(in.size(), 0ns);
    QVERIFY(back);
    for (std::size_t s = 0; s < in.size(); ++s) {
        std::int16_t v = 0;
        std::memcpy(&v, elementOf(*back, 0, s), sizeof(v));
        QCOMPARE(v, expected[s]);
    }
    QVERIFY(ring.reader.release(*back).ok());

    // Raw scaling saturates to the native type only.
    auto raw = ring.writer.beginWrite(2);
    const std::array<double, 2> big = {40000.0, -40000.0};
    AcqConvertOptions rawOptions;
    rawOptions.scaling = AcqScaling::Raw;
    auto rawResult = encode(AcqMatrixRef<const double>::rowMajor(big.data(), 1, 2), *raw, rawOptions);
    QVERIFY(rawResult);
    QCOMPARE(rawResult->clipped, std::uint64_t(2));
    ring.writer.commit(2);
    auto rawBack = ring.reader.acquire(2, 0ns);
    std::int16_t hi = 0, lo = 0;
    std::memcpy(&hi, elementOf(*rawBack, 0, 0), sizeof(hi));
    std::memcpy(&lo, elementOf(*rawBack, 0, 1), sizeof(lo));
    QCOMPARE(hi, std::int16_t(32767));
    QCOMPARE(lo, std::int16_t(-32768));
    QVERIFY(ring.reader.release(*rawBack).ok());
    QCOMPARE(std::fegetround(), FE_TONEAREST);
}

//=============================================================================================================

void TestAcqConvert::testEncodeAllFormatsBitExact()
{
    for (const AcqSampleFormat f : kFormats) {
        for (const AcqLayout layout : {AcqLayout::Interleaved, AcqLayout::Planar}) {
            for (const int digitalBits : {0, 2}) {
                for (const bool columnMajor : {false, true}) {
                    AcqStreamInfo info = makeInfo(f, layout, 5, digitalBits);
                    info.channels[2].rangeMin = -200.0;
                    info.channels[2].rangeMax = 300.0;
                    Ring ring = makeRing(info, 64, false);
                    Lcg rng{static_cast<std::uint64_t>(f) * 7 + static_cast<std::uint64_t>(layout)};
                    produceRandom(ring.writer, 50, rng); // wrap the next block, and leave digital bits in the ring
                    auto skip = ring.reader.acquire(50, 0ns);
                    QVERIFY(ring.reader.release(*skip).ok());

                    constexpr std::size_t kSamples = 30;
                    const double amplitude = isInteger(f) ? std::ldexp(1.0, wordBits(f) - 1) * 1.3 : 1e6;
                    std::vector<double> in(5 * kSamples);
                    for (double& v : in) {
                        v = rng.uniform(-amplitude, amplitude);
                    }
                    in[7] = std::numeric_limits<double>::quiet_NaN();
                    in[11] = 0.5;
                    in[12] = -1.5;

                    auto view = ring.writer.beginWrite(kSamples);
                    QVERIFY(view);
                    QCOMPARE(view->partCount(), std::size_t(2));
                    // Expected native bytes: start from what the ring holds (digital bits are kept), apply the reference.
                    std::vector<std::vector<std::byte>> expected(5 * kSamples);
                    std::uint64_t expectedClipped = 0;
                    for (std::size_t c = 0; c < 5; ++c) {
                        for (std::size_t s = 0; s < kSamples; ++s) {
                            std::vector<std::byte>& e = expected[c * kSamples + s];
                            std::size_t index = s;
                            std::size_t part = 0;
                            while (index >= view->part(part).samples) {
                                index -= view->part(part).samples;
                                ++part;
                            }
                            const std::byte* ringElement = view->part(part).at(c, index);
                            e.assign(ringElement, ringElement + componentBytes(f) * (isComplex(f) ? 2 : 1));
                            const AcqChannelInfo& ch = info.channels[c];
                            expectedClipped += refEncodeComponent(in[c * kSamples + s], f, 0, ch.digitalBits, true, ch.scale,
                                                                  ch.offset, ch.rangeMin, ch.rangeMax, e.data())
                                ? 1
                                : 0;
                            if (isComplex(f)) {
                                refEncodeComponent(0.0, f, 1, 0, true, ch.scale, ch.offset, ch.rangeMin, ch.rangeMax, e.data());
                            }
                        }
                    }
                    // The same values from a row-major and from a column-major matrix.
                    std::vector<double> transposed(in.size());
                    for (std::size_t c = 0; c < 5; ++c) {
                        for (std::size_t s = 0; s < kSamples; ++s) {
                            transposed[s * 5 + c] = in[c * kSamples + s];
                        }
                    }
                    auto result = encode(columnMajor ? AcqMatrixRef<const double>::columnMajor(transposed.data(), 5, kSamples)
                                                     : AcqMatrixRef<const double>::rowMajor(in.data(), 5, kSamples),
                                         *view);
                    QVERIFY2(result, sampleFormatName(f).data());
                    QCOMPARE(result->clipped, expectedClipped);
                    ring.writer.commit(kSamples);
                    auto back = ring.reader.acquire(kSamples, 0ns);
                    QVERIFY(back);
                    for (std::size_t c = 0; c < 5; ++c) {
                        for (std::size_t s = 0; s < kSamples; ++s) {
                            const std::vector<std::byte>& e = expected[c * kSamples + s];
                            QVERIFY2(std::memcmp(elementOf(*back, c, s), e.data(), e.size()) == 0,
                                     qPrintable(QString("%1 layout %2 channel %3 sample %4")
                                                    .arg(sampleFormatName(f).data())
                                                    .arg(static_cast<int>(layout))
                                                    .arg(c)
                                                    .arg(s)));
                        }
                    }
                    QVERIFY(ring.reader.release(*back).ok());

                    // Decoding what was encoded returns the input quantised to the channel's analog resolution.
                    if (!isComplex(f)) {
                        auto again = ring.writer.beginWrite(kSamples);
                        const std::array<double, 5> one = {1.0, 2.0, 3.0, 4.0, 5.0};
                        std::vector<double> column(5 * kSamples);
                        for (std::size_t s = 0; s < kSamples; ++s) {
                            for (std::size_t c = 0; c < 5; ++c) {
                                column[s * 5 + c] = one[c] * info.channels[c].scale * (isInteger(f) ? 64.0 : 1.0) +
                                    info.channels[c].offset;
                            }
                        }
                        QVERIFY(encode(AcqMatrixRef<const double>::columnMajor(column.data(), 5, kSamples), *again));
                        ring.writer.commit(kSamples);
                        auto decoded = ring.reader.acquire(kSamples, 0ns);
                        std::vector<double> roundTrip(5 * kSamples);
                        QVERIFY(decode(*decoded, AcqMatrixRef<double>::columnMajor(roundTrip.data(), 5, kSamples)).ok());
                        for (std::size_t i = 0; i < column.size(); ++i) {
                            QVERIFY2(std::abs(roundTrip[i] - column[i]) <= 1e-9 * std::abs(column[i]) + 1e-9,
                                     qPrintable(QString("%1 element %2: %3 != %4")
                                                    .arg(sampleFormatName(f).data())
                                                    .arg(i)
                                                    .arg(roundTrip[i])
                                                    .arg(column[i])));
                        }
                        QVERIFY(ring.reader.release(*decoded).ok());
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void TestAcqConvert::testEncodeComplexAndOffsets()
{
    // Complex input into a real stream: a non-zero imaginary part cannot be represented.
    AcqStreamInfo real = makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 1, 0);
    real.channels[0].scale = 2.0;
    real.channels[0].offset = 1.0;
    Ring ring = makeRing(real, 64, false);
    const std::array<std::complex<float>, 3> z = {{{5.0F, 0.0F}, {7.0F, 1.0F}, {-1.0F, 0.0F}}};
    auto view = ring.writer.beginWrite(3);
    auto result = encode(AcqMatrixRef<const std::complex<float>>::rowMajor(z.data(), 1, 3), *view);
    QVERIFY(result);
    QCOMPARE(result->clipped, std::uint64_t(1));
    ring.writer.commit(3);
    auto back = ring.reader.acquire(3, 0ns);
    std::array<float, 3> values{};
    QVERIFY(decode(*back, AcqMatrixRef<float>::rowMajor(values.data(), 1, 3)).ok());
    QCOMPARE(values[0], 5.0F);
    QCOMPARE(values[1], 7.0F);
    QCOMPARE(values[2], -1.0F);
    QVERIFY(ring.reader.release(*back).ok());

    // Complex int16 I/Q: the offset shifts the in-phase component only.
    AcqStreamInfo iq = makeInfo(AcqSampleFormat::ComplexInt16, AcqLayout::Interleaved, 1, 0);
    iq.channels[0].scale = 0.5;
    iq.channels[0].offset = 10.0;
    Ring iqRing = makeRing(iq, 64, false);
    const std::array<std::complex<float>, 2> w = {{{12.0F, -3.0F}, {10.0F, 16383.75F}}};
    auto iqView = iqRing.writer.beginWrite(2);
    auto iqResult = encode(AcqMatrixRef<const std::complex<float>>::rowMajor(w.data(), 1, 2), *iqView);
    QVERIFY(iqResult);
    QCOMPARE(iqResult->clipped, std::uint64_t(1));
    iqRing.writer.commit(2);
    auto iqBack = iqRing.reader.acquire(2, 0ns);
    AcqComplexInt16 first{};
    AcqComplexInt16 second{};
    std::memcpy(&first, elementOf(*iqBack, 0, 0), sizeof(first));
    std::memcpy(&second, elementOf(*iqBack, 0, 1), sizeof(second));
    QCOMPARE(first.re, std::int16_t(4));
    QCOMPARE(first.im, std::int16_t(-6));
    QCOMPARE(second.re, std::int16_t(0));
    QCOMPARE(second.im, std::int16_t(32767));
    std::array<std::complex<float>, 2> decoded{};
    QVERIFY(decode(*iqBack, AcqMatrixRef<std::complex<float>>::rowMajor(decoded.data(), 1, 2)).ok());
    QCOMPARE(decoded[0], std::complex<float>(12.0F, -3.0F));
    QVERIFY(iqRing.reader.release(*iqBack).ok());
}

//=============================================================================================================

void TestAcqConvert::testDigitalBitsSpectrumPacking()
{
    // Spectrum AWG: an int16 channel with the T/R switch on bit 15 and RF blanking on bit 14
    // (SPCM_XMODE_DIGOUTSRC_BIT15/BIT14); the analog value is the low 14 bits at full-word scale.
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 2, 0);
    info.channels[0].scale = 1.0 / 32768.0;
    info.channels[0].offset = 0.0;
    info.channels[0].digitalBits = 2;
    info.bitFields = {{"tr_switch", 0, 15, 1, AcqChannelKind::RfSwitch}, {"rf_blank", 0, 14, 1, AcqChannelKind::TriggerOut}};
    Ring ring = makeRing(info, 64, false);

    constexpr std::size_t kSamples = 4;
    auto view = ring.writer.beginWrite(kSamples);
    QVERIFY(view);
    const std::array<double, 2 * kSamples> analog = {0.5, -0.25, 0.999, -1.0, 0.0, 0.0, 0.0, 0.0};
    QVERIFY(encode(AcqMatrixRef<const double>::rowMajor(analog.data(), 2, kSamples), *view));
    const std::array<std::uint32_t, 2 * kSamples> lines = {1, 0, 1, 0, 0, 1, 1, 0};
    auto bits = encodeBits(AcqMatrixRef<const std::uint32_t>::rowMajor(lines.data(), 2, kSamples), info.bitFields, *view);
    QVERIFY(bits);
    QCOMPARE(bits->clipped, std::uint64_t(0));
    // Writing the analog value again keeps the lines (encode() leaves the top digitalBits bits alone).
    QVERIFY(encode(AcqMatrixRef<const double>::rowMajor(analog.data(), 2, kSamples), *view));
    const std::array<std::uint32_t, 2> narrow = {1, 0};
    QCOMPARE(encodeBits(AcqMatrixRef<const std::uint32_t>::rowMajor(narrow.data(), 2, 1), info.bitFields, *view)
                 .error()
                 .code(),
             AcqErrorCode::ShapeMismatch);
    ring.writer.commit(kSamples);

    auto back = ring.reader.acquire(kSamples, 0ns);
    QVERIFY(back);
    // Word = (tr << 15) | (blank << 14) | (analog * 32768 >> 2 in 14 bits).
    const std::array<std::uint16_t, kSamples> words = {0x8000 | 0x1000, 0x4000 | 0x3800, 0xC000 | 0x1FF8, 0x2000};
    for (std::size_t s = 0; s < kSamples; ++s) {
        std::uint16_t w = 0;
        std::memcpy(&w, elementOf(*back, 0, s), sizeof(w));
        QCOMPARE(w, words[s]);
    }
    std::array<std::uint32_t, 2 * kSamples> decodedLines{};
    QVERIFY(decodeBits(*back, info.bitFields, AcqMatrixRef<std::uint32_t>::rowMajor(decodedLines.data(), 2, kSamples))
                .ok());
    QVERIFY(decodedLines == lines);
    std::array<double, 2 * kSamples> decodedAnalog{};
    QVERIFY(decode(*back, AcqMatrixRef<double>::rowMajor(decodedAnalog.data(), 2, kSamples)).ok());
    QCOMPARE(decodedAnalog[0], 0.5);
    QCOMPARE(decodedAnalog[1], -0.25);
    QCOMPARE(decodedAnalog[2], 8184.0 * 4.0 / 32768.0); // 0.999 quantised to the 14 analog bits
    QCOMPARE(decodedAnalog[3], -1.0);

    const std::array<AcqBitField, 1> outside = {{{"bad", 0, 15, 2, AcqChannelKind::TriggerOut}}};
    QCOMPARE(decodeBits(*back, outside, AcqMatrixRef<std::uint32_t>::rowMajor(decodedLines.data(), 1, kSamples)).code(),
             AcqErrorCode::InvalidParameter);
    const std::array<AcqBitField, 1> noChannel = {{{"bad", 5, 0, 1, AcqChannelKind::TriggerOut}}};
    QCOMPARE(
        decodeBits(*back, noChannel, AcqMatrixRef<std::uint32_t>::rowMajor(decodedLines.data(), 1, kSamples)).code(),
        AcqErrorCode::InvalidParameter);
    QVERIFY(ring.reader.release(*back).ok());

    // A value wider than its field is truncated to the field and counted.
    auto next = ring.writer.beginWrite(1);
    const std::array<std::uint32_t, 2> tooWide = {2, 1};
    auto clipped = encodeBits(AcqMatrixRef<const std::uint32_t>::rowMajor(tooWide.data(), 2, 1), info.bitFields, *next);
    QVERIFY(clipped);
    QCOMPARE(clipped->clipped, std::uint64_t(1));
    std::uint16_t truncated = 0;
    std::memcpy(&truncated, next->part(0).at(0, 0), sizeof(truncated));
    QCOMPARE(truncated & 0xC000, 0x4000);
    ring.writer.commit(1);

    Ring floats = makeRing(makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 1, 0), 64, false);
    Lcg rng{3};
    produceRandom(floats.writer, 2, rng);
    auto floatView = floats.reader.acquire(2, 0ns);
    QCOMPARE(decodeBits(*floatView, info.bitFields, AcqMatrixRef<std::uint32_t>::rowMajor(decodedLines.data(), 2, 2))
                 .code(),
             AcqErrorCode::UnsupportedFormat);
    QVERIFY(floats.reader.release(*floatView).ok());
}

//=============================================================================================================

void TestAcqConvert::testDecodePart()
{
    const AcqStreamInfo info = makeInfo(AcqSampleFormat::Int24Packed, AcqLayout::Planar, 3, 0);
    Ring ring = makeRing(info, 64, false);
    Lcg rng{9};
    produceRandom(ring.writer, 20, rng);
    auto view = ring.reader.acquire(20, 0ns);
    QVERIFY(view);
    std::vector<float> fromPart(3 * 20);
    std::vector<float> fromView(3 * 20);
    std::vector<double> scales;
    std::vector<double> offsets;
    for (const AcqChannelInfo& ch : info.channels) {
        scales.push_back(ch.scale);
        offsets.push_back(ch.offset);
    }
    QVERIFY(decodePart(view->part(0), AcqSampleFormat::Int24Packed, scales, offsets,
                       AcqMatrixRef<float>::columnMajor(fromPart.data(), 3, 20), AcqScaling::Physical)
                .ok());
    QVERIFY(decode(*view, AcqMatrixRef<float>::columnMajor(fromView.data(), 3, 20)).ok());
    QVERIFY(fromPart == fromView);
    QCOMPARE(decodePart(view->part(0), AcqSampleFormat::Int24Packed, std::span<const double>(scales).first(2), offsets,
                        AcqMatrixRef<float>::columnMajor(fromPart.data(), 3, 20), AcqScaling::Physical)
                 .code(),
             AcqErrorCode::ShapeMismatch);
    QVERIFY(decodePart(view->part(0), AcqSampleFormat::Int24Packed, {}, {},
                       AcqMatrixRef<float>::columnMajor(fromPart.data(), 3, 20), AcqScaling::Raw)
                .ok());
    QCOMPARE(decodePart(view->part(0), AcqSampleFormat::ComplexInt16, scales, offsets,
                        AcqMatrixRef<float>::columnMajor(fromPart.data(), 3, 20), AcqScaling::Raw)
                 .code(),
             AcqErrorCode::UnsupportedFormat);
    QVERIFY(ring.reader.release(*view).ok());
}

//=============================================================================================================

void TestAcqConvert::testReaderRead()
{
    const AcqStreamInfo info = makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 4, 0);
    Ring ring = makeRing(info, 64, false);
    Lcg rng{11};
    produceRandom(ring.writer, 30, rng);
    std::vector<float> block(4 * 20);
    QCOMPARE(ring.reader.read(AcqMatrixRef<float>::columnMajor(block.data(), 3, 20), 0ns).error().code(),
             AcqErrorCode::ShapeMismatch);
    auto first = ring.reader.read(AcqMatrixRef<float>::columnMajor(block.data(), 4, 20), 0ns);
    QVERIFY(first);
    QCOMPARE(first->firstSample, std::uint64_t(0));
    QCOMPARE(first->samples, std::size_t(20));
    QCOMPARE(first->lost, std::uint64_t(0));
    QCOMPARE(ring.reader.read(AcqMatrixRef<float>::columnMajor(block.data(), 4, 20), 0ns).error().code(),
             AcqErrorCode::Timeout);

    // Overwritten samples are reported, the rest is read; after finish() a short final block is returned.
    produceRandom(ring.writer, 50, rng);
    produceRandom(ring.writer, 50, rng);
    std::vector<double> wide(4 * 20);
    auto later = ring.reader.read(AcqMatrixRef<double>::columnMajor(wide.data(), 4, 20), 0ns);
    QVERIFY(later);
    QCOMPARE(later->firstSample, std::uint64_t(66));
    QCOMPARE(later->lost, std::uint64_t(46));
    ring.writer.finish();
    std::vector<std::complex<float>> rest(4 * 64);
    auto tail = ring.reader.read(AcqMatrixRef<std::complex<float>>::columnMajor(rest.data(), 4, 64), 0ns);
    QVERIFY(tail);
    QCOMPARE(tail->samples, std::size_t(44));
    QCOMPARE(ring.reader.read(AcqMatrixRef<float>::columnMajor(block.data(), 4, 20), 0ns).error().code(),
             AcqErrorCode::EndOfStream);
    QCOMPARE(AcqReader().read(AcqMatrixRef<float>::columnMajor(block.data(), 4, 20), 0ns).error().code(),
             AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqConvert::testDeviceMemoryRefused()
{
    // GPU memory is for GPU consumers: the converters refuse it instead of reading it on the CPU.
    constexpr std::size_t kCapacity = 64;
    const AcqStreamInfo info = makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 2, 0);
    std::vector<std::byte> gpu(kCapacity * info.frameBytes() + 4096);
    AcqStreamOptions o;
    o.lockMemory = false;
    const auto misalignment = reinterpret_cast<std::uintptr_t>(gpu.data()) % 4096;
    o.external.data = gpu.data() + (misalignment == 0 ? 0 : 4096 - misalignment);
    o.external.bytes = kCapacity * info.frameBytes();
    o.external.memory = {AcqMemoryKind::Device, 0};
    auto writer = AcqWriter::create(info, o);
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(writer->grant(8).ok());
    writer->commit(8);
    auto view = reader->acquire(8, 0ns);
    QVERIFY(view);
    std::vector<float> out(2 * 8);
    QCOMPARE(decode(*view, AcqMatrixRef<float>::columnMajor(out.data(), 2, 8)).code(), AcqErrorCode::DeviceMemory);
    std::array<std::uint32_t, 8> lines{};
    const std::array<AcqBitField, 1> field = {{{"sync", 0, 15, 1, AcqChannelKind::TriggerOut}}};
    QCOMPARE(decodeBits(*view, field, AcqMatrixRef<std::uint32_t>::rowMajor(lines.data(), 1, 8)).code(),
             AcqErrorCode::DeviceMemory);
    QVERIFY(reader->release(*view).ok());
    // read() refuses at once, without waiting for samples it could never copy.
    QCOMPARE(reader->read(AcqMatrixRef<float>::columnMajor(out.data(), 2, 8), 1h).error().code(),
             AcqErrorCode::DeviceMemory);
    QVERIFY(writer->grant(8).ok());
    writer->commit(8);
    QCOMPARE(reader->read(AcqMatrixRef<float>::columnMajor(out.data(), 2, 8), 0ns).error().code(),
             AcqErrorCode::DeviceMemory);
    QCOMPARE(reader->stats().position, std::uint64_t(8)); // refused before acquiring, nothing consumed

    auto write = writer->beginWrite(4);
    QVERIFY(write);
    const std::array<float, 8> values{};
    QCOMPARE(encode(AcqMatrixRef<const float>::rowMajor(values.data(), 2, 4), *write).error().code(),
             AcqErrorCode::DeviceMemory);
    QCOMPARE(encodeBits(AcqMatrixRef<const std::uint32_t>::rowMajor(lines.data(), 1, 4), field, *write).error().code(),
             AcqErrorCode::DeviceMemory);
}

//=============================================================================================================

void TestAcqConvert::testFloatSaturationAndWideBitFields()
{
    // Values beyond the native floating-point range saturate to +-max and are counted.
    AcqStreamInfo f32 = makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 1, 0);
    f32.channels[0].scale = 1.0;
    f32.channels[0].offset = 0.0;
    Ring floats = makeRing(f32, 64, false);
    const std::array<double, 3> huge = {1e300, -1e300, 1.0};
    auto fv = floats.writer.beginWrite(3);
    auto fr = encode(AcqMatrixRef<const double>::rowMajor(huge.data(), 1, 3), *fv);
    QVERIFY(fr);
    QCOMPARE(fr->clipped, std::uint64_t(2));
    std::array<float, 3> written{};
    for (std::size_t s = 0; s < 3; ++s) {
        std::memcpy(&written[s], fv->part(0).at(0, s), sizeof(float));
    }
    QCOMPARE(written[0], std::numeric_limits<float>::max());
    QCOMPARE(written[1], -std::numeric_limits<float>::max());
    QCOMPARE(written[2], 1.0F);
    // The encoder needs one column per reserved sample.
    QCOMPARE(encode(AcqMatrixRef<const double>::rowMajor(huge.data(), 1, 2), *fv).error().code(),
             AcqErrorCode::ShapeMismatch);
    QCOMPARE(encode(AcqMatrixRef<const double>{}, *fv).error().code(), AcqErrorCode::ShapeMismatch);
    floats.writer.commit(3);

    AcqStreamInfo f64 = makeInfo(AcqSampleFormat::Float64, AcqLayout::Interleaved, 1, 0);
    f64.channels[0].scale = 1.0;
    f64.channels[0].offset = 0.0;
    Ring doubles = makeRing(f64, 64, false);
    const std::array<double, 2> infinite = {std::numeric_limits<double>::infinity(),
                                            -std::numeric_limits<double>::infinity()};
    auto dv = doubles.writer.beginWrite(2);
    AcqConvertOptions raw;
    raw.scaling = AcqScaling::Raw;
    auto dr = encode(AcqMatrixRef<const double>::rowMajor(infinite.data(), 1, 2), *dv, raw);
    QVERIFY(dr);
    QCOMPARE(dr->clipped, std::uint64_t(2));
    double largest = 0.0;
    std::memcpy(&largest, dv->part(0).at(0, 0), sizeof(largest));
    QCOMPARE(largest, std::numeric_limits<double>::max());
    doubles.writer.commit(2);

    // Bit fields sit at the top of 24- and 32-bit words as well.
    for (const AcqSampleFormat format : {AcqSampleFormat::Int24Packed, AcqSampleFormat::Int32}) {
        const auto top = static_cast<std::uint8_t>(format == AcqSampleFormat::Int32 ? 31 : 23);
        AcqStreamInfo info = makeInfo(format, AcqLayout::Planar, 1, 0);
        info.channels[0].digitalBits = 1;
        info.bitFields = {{"sync", 0, top, 1, AcqChannelKind::ClockSync}};
        Ring ring = makeRing(info, 64, false);
        auto view = ring.writer.beginWrite(4);
        const std::array<std::uint32_t, 4> lines = {1, 0, 0, 1};
        auto bits = encodeBits(AcqMatrixRef<const std::uint32_t>::rowMajor(lines.data(), 1, 4), info.bitFields, *view);
        QVERIFY(bits);
        QCOMPARE(bits->clipped, std::uint64_t(0));
        ring.writer.commit(4);
        auto back = ring.reader.acquire(4, 0ns);
        std::array<std::uint32_t, 4> decoded{};
        QVERIFY(decodeBits(*back, info.bitFields, AcqMatrixRef<std::uint32_t>::rowMajor(decoded.data(), 1, 4)).ok());
        QVERIFY(decoded == lines);
        QVERIFY(ring.reader.release(*back).ok());
    }
}

//=============================================================================================================

void TestAcqConvert::testWideStreams()
{
    // 300 channels span several channel chunks (256) and transpose tiles (32) of the converters.
    constexpr std::size_t kChannels = 300;
    for (const AcqSampleFormat f : {AcqSampleFormat::Int16, AcqSampleFormat::Float32, AcqSampleFormat::ComplexInt16}) {
        for (const AcqLayout layout : {AcqLayout::Interleaved, AcqLayout::Planar}) {
            const AcqStreamInfo info = makeInfo(f, layout, kChannels, 0);
            Ring ring = makeRing(info, 128, false);
            Lcg rng{static_cast<std::uint64_t>(f) * 13 + static_cast<std::uint64_t>(layout)};
            produceRandom(ring.writer, 70, rng);
            auto view = ring.reader.acquire(70, 0ns);
            QVERIFY(view);
            std::string failure = checkDecode<std::complex<float>>(*view, info);
            QVERIFY2(failure.empty(), failure.c_str());
            if (!isComplex(f)) {
                failure = checkDecode<float>(*view, info);
                QVERIFY2(failure.empty(), failure.c_str());
            }
            QVERIFY(ring.reader.release(*view).ok());

            if (isComplex(f)) {
                continue;
            }
            // Exactly representable values come back unchanged through encode() and decode().
            constexpr std::size_t kSamples = 40;
            std::vector<double> in(kChannels * kSamples);
            for (std::size_t i = 0; i < in.size(); ++i) {
                const std::size_t c = i % kChannels;
                in[i] = static_cast<double>(static_cast<int>(i % 2001) - 1000) * info.channels[c].scale + info.channels[c].offset;
            }
            auto write = ring.writer.beginWrite(kSamples);
            auto encoded = encode(AcqMatrixRef<const double>::columnMajor(in.data(), kChannels, kSamples), *write);
            QVERIFY(encoded);
            QCOMPARE(encoded->clipped, std::uint64_t(0));
            ring.writer.commit(kSamples);
            auto back = ring.reader.acquire(kSamples, 0ns);
            std::vector<double> out(in.size());
            QVERIFY(decode(*back, AcqMatrixRef<double>::columnMajor(out.data(), kChannels, kSamples)).ok());
            for (std::size_t i = 0; i < in.size(); ++i) {
                QVERIFY2(std::abs(out[i] - in[i]) <= 1e-9 * std::abs(in[i]) + 1e-12,
                         qPrintable(QString("%1 element %2").arg(sampleFormatName(f).data()).arg(i)));
            }
            QVERIFY(ring.reader.release(*back).ok());
        }
    }
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqConvert)
#include "test_acq_convert.moc"
