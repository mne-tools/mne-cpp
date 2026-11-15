//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_fiff_replay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests for the fiff:// source: native formats bit-exact to FiffRawData, skips, events, looping,
 *           start offset, pacing, parameter errors and a recording of a replay.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_device.h>
#include <acq/acq_device_registry.h>
#include <acq/acq_reader.h>
#include <acq/acq_stream.h>
#include <acq/fiff/acq_fiff.h>
#include <acq/fiff/acq_fiff_replay.h>
#include <acq/fiff/acq_recorder.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

constexpr int kBuffer = 50;  // samples per data buffer of the fixtures
constexpr int kChannels = 3; // two EEG channels and STI 014
constexpr int kLength = 250; // four data buffers and one skipped buffer

/** Trigger value of the fixtures: held from sample 0 (no event), two adjacent codes, one in the last buffer. */
int trigger(int sample)
{
    if (sample < 5) {
        return 3;
    }
    if (sample >= 10 && sample < 15) {
        return 1;
    }
    if (sample >= 15 && sample < 20) {
        return 2;
    }
    if (sample >= 160 && sample < 170) {
        return 4;
    }
    return sample >= 245 ? 5 : 0;
}

constexpr std::array<AcqEvent, 4> kEvents = {{{10, 5, 1}, {15, 5, 2}, {160, 10, 4}, {245, 5, 5}}};

/** Stored value of channel @p c at file sample @p s (exact in every FIFF type). */
double storedValue(fiff_int_t type, int s, int c)
{
    if (c == kChannels - 1) {
        return trigger(s);
    }
    const double v = static_cast<double>((s * 37 + c * 101) % 2000 - 1000);
    return type == FIFFT_FLOAT || type == FIFFT_DOUBLE ? v * 0.25 : v;
}

/** Writes a raw file of @p type: buffers 0, 1, a one-buffer skip, buffers 3, 4 (zeros at 100-149). */
bool writeFixture(const QString& path, fiff_int_t type, fiff_int_t lastType = -1)
{
    AcqStreamInfo stream;
    stream.sfreq = 1000.0;
    for (const char* name : {"EEG 001", "EEG 002"}) {
        AcqChannelInfo channel;
        channel.name = name;
        channel.kind = AcqChannelKind::Eeg;
        channel.unit = AcqUnit::Volt;
        stream.channels.push_back(channel);
    }
    AcqChannelInfo stim;
    stim.name = "STI 014";
    stim.kind = AcqChannelKind::Stim;
    stim.unit = AcqUnit::None;
    stream.channels.push_back(stim);
    auto info = toFiffInfo(stream);
    if (!info) {
        return false;
    }
    info->chs[0].cal = 0.3F; // range * cal is not exact in float: scale must be computed in double
    info->chs[0].range = 2.7F;
    info->chs[1].cal = 1e-6F;
    Eigen::RowVectorXd cals;
    auto out = FiffStream::start_writing_raw(path.toStdString(), *info, cals, defaultMatrixXi, false, type);
    if (!out) {
        return false;
    }
    for (int b = 0; b < kLength / kBuffer; ++b) {
        if (b == 4 && lastType != -1) {
            type = lastType;
        }
        const std::size_t size = type == FIFFT_DAU_PACK16 || type == FIFFT_SHORT ? 2U : (type == FIFFT_DOUBLE ? 8U : 4U);
        if (b == 2) {
            const fiff_int_t one = 1;
            out->write_int(FIFF_DATA_SKIP, &one);
            continue;
        }
        FiffTag tag;
        tag.kind = FIFF_DATA_BUFFER;
        tag.type = type;
        tag.next = FIFFV_NEXT_SEQ;
        tag.resize(std::size_t{kBuffer} * std::size_t{kChannels} * size);
        std::byte* at = tag.data();
        for (int s = b * kBuffer; s < (b + 1) * kBuffer; ++s) {
            for (int c = 0; c < kChannels; ++c, at += size) {
                const double v = storedValue(type, s, c);
                if (size == 2) {
                    const auto x = static_cast<std::int16_t>(v);
                    std::memcpy(at, &x, 2);
                } else if (type == FIFFT_INT) {
                    const auto x = static_cast<std::int32_t>(v);
                    std::memcpy(at, &x, 4);
                } else if (type == FIFFT_FLOAT) {
                    const auto x = static_cast<float>(v);
                    std::memcpy(at, &x, 4);
                } else {
                    std::memcpy(at, &v, 8);
                }
            }
        }
        out->write_tag(tag);
    }
    out->finish_writing_raw();
    return true;
}

/** Everything a gating reader delivered: physical values (scale * stored) per frame, and events. */
struct Replayed
{
    AcqSampleFormat format = AcqSampleFormat::Float32;
    std::vector<double> values; // frame-major
    std::vector<AcqEvent> events;
    std::uint64_t lost = 0;
    AcqDeviceDescription description;
};

double readStored(AcqSampleFormat format, const std::byte* at)
{
    switch (format) {
        case AcqSampleFormat::Int16: {
            std::int16_t v = 0;
            std::memcpy(&v, at, sizeof(v));
            return v;
        }
        case AcqSampleFormat::Int32: {
            std::int32_t v = 0;
            std::memcpy(&v, at, sizeof(v));
            return v;
        }
        case AcqSampleFormat::Float32: {
            float v = 0.0F;
            std::memcpy(&v, at, sizeof(v));
            return v;
        }
        default: {
            double v = 0.0;
            std::memcpy(&v, at, sizeof(v));
            return v;
        }
    }
}

/** Replays @p uri through a gating Lossless reader until end of stream or @p limit frames. */
Replayed replay(const std::string& uri, std::uint64_t limit = 0)
{
    Replayed out;
    auto device = AcqDevice::open(uri);
    if (!device) {
        qWarning("open(%s): %s", uri.c_str(), device.error().text().c_str());
        return out;
    }
    out.description = device->description();
    AcqReaderOptions options;
    options.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, options);
    if (!reader || !device->start().ok()) {
        return out;
    }
    const auto info = reader->stream().info();
    out.format = info->format;
    while (limit == 0 || out.values.size() < limit * info->channels.size()) {
        auto view = reader->acquire(1, 5s);
        if (!view) {
            break;
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i) {
                for (std::size_t c = 0; c < info->channels.size(); ++c) {
                    out.values.push_back(info->channels[c].scale * readStored(info->format, part.at(c, i)));
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            out.events.push_back(view->events()[e]);
        }
        static_cast<void>(reader->release(*view));
    }
    out.lost = reader->stats().lost;
    device->close();
    return out;
}

/** @return read_raw_segment of the whole file, frame-major. */
std::vector<double> reference(const QString& path)
{
    FiffRawData raw(path.toStdString());
    Eigen::MatrixXd data, times;
    if (!raw.read_raw_segment(data, times)) {
        return {};
    }
    return {data.data(), data.data() + data.size()};
}

bool bitEqual(const std::vector<double>& a, const std::vector<double>& b)
{
    if (a.size() != b.size()) {
        qWarning("sizes differ: %zu vs %zu", a.size(), b.size());
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::bit_cast<std::uint64_t>(a[i]) != std::bit_cast<std::uint64_t>(b[i])) {
            qWarning("first difference at %zu: %.17g vs %.17g", i, a[i], b[i]);
            return false;
        }
    }
    return true;
}

std::string uriOf(const QString& path, const char* query = "realtime=0")
{
    return "fiff://" + path.toStdString() + "?" + query;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestAcqFiffReplay
 */
class TestAcqFiffReplay : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void replaysNativeFormats_data();
    void replaysNativeFormats();
    void loopsFromStartOffset();
    void pacesInRealTime();
    void rejectsBadUris();
    void recordsReplayBitExact();
    void replaysSampleData();

private:
    QTemporaryDir m_dir;
    QString m_float;
};

//=============================================================================================================

void TestAcqFiffReplay::initTestCase()
{
    QVERIFY(m_dir.isValid());
    QVERIFY(registerFiffScheme().ok());
    QVERIFY(registerFiffScheme().ok()); // repeated calls are harmless
    m_float = m_dir.filePath(QStringLiteral("float_raw.fif"));
    QVERIFY(writeFixture(m_float, FIFFT_FLOAT));
}

//=============================================================================================================

void TestAcqFiffReplay::replaysNativeFormats_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("format");
    QTest::newRow("dau_pack16") << int(FIFFT_DAU_PACK16) << int(AcqSampleFormat::Int16);
    QTest::newRow("short") << int(FIFFT_SHORT) << int(AcqSampleFormat::Int16);
    QTest::newRow("int") << int(FIFFT_INT) << int(AcqSampleFormat::Int32);
    QTest::newRow("float") << int(FIFFT_FLOAT) << int(AcqSampleFormat::Float32);
    QTest::newRow("double") << int(FIFFT_DOUBLE) << int(AcqSampleFormat::Float64);
}

void TestAcqFiffReplay::replaysNativeFormats()
{
    QFETCH(int, type);
    QFETCH(int, format);
    const QString path = m_dir.filePath(QString("%1_raw.fif").arg(QTest::currentDataTag()));
    QVERIFY(writeFixture(path, type));
    const std::vector<double> expected = reference(path);
    QCOMPARE(expected.size(), std::size_t(kLength * kChannels));

    const Replayed r = replay(uriOf(path));
    QCOMPARE(int(r.format), format);
    QCOMPARE(r.lost, std::uint64_t(0));
    QVERIFY(bitEqual(r.values, expected)); // scale * stored == read_raw_segment, skip included
    QCOMPARE(r.values[120 * kChannels], 0.0);
    QCOMPARE(r.events.size(), kEvents.size());
    for (std::size_t i = 0; i < kEvents.size(); ++i) {
        QCOMPARE(r.events[i].sample, kEvents[i].sample);
        QCOMPARE(r.events[i].duration, kEvents[i].duration);
        QCOMPARE(r.events[i].code, kEvents[i].code);
    }
    QCOMPARE(r.description.backend, std::string("fiff"));
    QCOMPARE(r.description.device, path.toStdString());
}

//=============================================================================================================

void TestAcqFiffReplay::loopsFromStartOffset()
{
    // Stream sample 0 is file sample 150; at the end the replay continues at file sample 0.
    const std::vector<double> file = reference(m_float);
    const std::uint64_t frames = 700;
    const Replayed r = replay(uriOf(m_float, "realtime=0&loop=1&start=0.15&block=7"), frames);
    QVERIFY(r.values.size() >= frames * kChannels);
    std::vector<double> expectedValues;
    for (std::uint64_t s = 0; s < frames; ++s) {
        const std::uint64_t f = (150 + s) % kLength;
        expectedValues.insert(expectedValues.end(), file.begin() + static_cast<std::ptrdiff_t>(f * kChannels),
                              file.begin() + static_cast<std::ptrdiff_t>((f + 1) * kChannels));
    }
    QVERIFY(bitEqual(std::vector<double>(r.values.begin(), r.values.begin() + static_cast<std::ptrdiff_t>(frames * kChannels)),
                     expectedValues));
    std::vector<std::uint64_t> onsets;
    for (const AcqEvent& event : r.events) {
        if (event.sample < frames) {
            onsets.push_back(event.sample);
        }
    }
    // 160 and 245 of the first pass, then every event of each further pass (pass length 250).
    const std::vector<std::uint64_t> expected = {10, 95, 110, 115, 260, 345, 360, 365, 510, 595, 610, 615};
    QCOMPARE(onsets, expected);

    const Replayed silent = replay(uriOf(m_float, "realtime=0&events=0"));
    QVERIFY(silent.events.empty());
    QCOMPARE(silent.values.size(), std::size_t(kLength * kChannels));
}

//=============================================================================================================

void TestAcqFiffReplay::pacesInRealTime()
{
    // 250 samples at 1000 Hz and speed 5 last 50 ms.
    const auto begin = std::chrono::steady_clock::now();
    const Replayed r = replay(uriOf(m_float, "realtime=1&speed=5"));
    const auto elapsed = std::chrono::steady_clock::now() - begin;
    QCOMPARE(r.values.size(), std::size_t(kLength * kChannels));
    QVERIFY(elapsed >= 45ms);

    // stop() wakes a paced producer at once.
    auto device = AcqDevice::open(uriOf(m_float, "realtime=1&speed=0.01&loop=1"));
    QVERIFY(device);
    QVERIFY(device->start().ok());
    const auto stopping = std::chrono::steady_clock::now();
    device->close();
    QVERIFY(std::chrono::steady_clock::now() - stopping < 2s);
}

//=============================================================================================================

void TestAcqFiffReplay::rejectsBadUris()
{
    const auto code = [](const std::string& uri) {
        auto device = AcqDevice::open(uri);
        return device ? AcqErrorCode::None : device.error().code();
    };
    QCOMPARE(code("fiff://"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code(uriOf(m_dir.filePath("missing_raw.fif"))), AcqErrorCode::DeviceNotFound);
    QCOMPARE(code(uriOf(m_float, "start=0.25")), AcqErrorCode::InvalidParameter);
    QCOMPARE(code(uriOf(m_float, "stim=STI%20999")), AcqErrorCode::InvalidParameter);
    QCOMPARE(code(uriOf(m_float, "speed=0")), AcqErrorCode::InvalidParameter);
    QCOMPARE(code(uriOf(m_float, "colour=red")), AcqErrorCode::InvalidParameter);
    QCOMPARE(code(uriOf(m_float, "stim=EEG%20001&realtime=0")), AcqErrorCode::None);
    QCOMPARE(code(uriOf(m_float, "stim=sti014&realtime=0")), AcqErrorCode::None); // names match as in MNE-C

    const QString text = m_dir.filePath(QStringLiteral("not_raw.fif"));
    QFile file(text);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not a FIFF file");
    file.close();
    QCOMPARE(code(uriOf(text)), AcqErrorCode::UnsupportedFormat);
    const QString mixed = m_dir.filePath(QStringLiteral("mixed_raw.fif"));
    QVERIFY(writeFixture(mixed, FIFFT_FLOAT, FIFFT_INT));
    QCOMPARE(code(uriOf(mixed)), AcqErrorCode::UnsupportedFormat);

    bool listed = false;
    for (const AcqSchemeInfo& scheme : AcqDeviceRegistry::instance().schemes()) {
        listed = listed || scheme.scheme == "fiff";
    }
    QVERIFY(listed);
}

//=============================================================================================================

void TestAcqFiffReplay::recordsReplayBitExact()
{
    // record(replay(file)) keeps native samples and the file's cal and range, so both read back identically.
    const QString path = m_dir.filePath(QStringLiteral("dau_raw.fif"));
    QVERIFY(writeFixture(path, FIFFT_DAU_PACK16));
    auto device = AcqDevice::open(uriOf(path));
    QVERIFY(device);
    AcqRecorderOptions options;
    options.device = device->description();
    const QString copy = m_dir.filePath(QStringLiteral("copy_raw.fif"));
    auto recorder = AcqRecorder::start(device->stream(), copy.toStdString(), options);
    QVERIFY(recorder);
    QVERIFY(device->start().ok());
    QTRY_VERIFY_WITH_TIMEOUT(!recorder->isRecording(), 10000);
    device->close();
    QVERIFY(recorder->stop().ok());
    QCOMPARE(recorder->stats().lost, std::uint64_t(0));
    QVERIFY(bitEqual(reference(copy), reference(path)));
}

//=============================================================================================================

void TestAcqFiffReplay::replaysSampleData()
{
    const QString path = QCoreApplication::applicationDirPath() +
        "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(path)) {
        QSKIP("MNE-CPP test data not found");
    }
    const Replayed r = replay(uriOf(path, "realtime=0&block=1000"));
    QVERIFY(bitEqual(r.values, reference(path)));
    QVERIFY(!r.events.empty());
}

QTEST_GUILESS_MAIN(TestAcqFiffReplay)
#include "test_acq_fiff_replay.moc"
