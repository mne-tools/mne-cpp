//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_recorder.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the FIFF bridge of mne_acq: FiffInfo conversion and AcqRecorder.
 *
 * Every recording is read back by MNE-CPP and, through check_recording.py, by mne.io.read_raw_fif with
 * warnings as errors; the data must equal what a second Lossless reader of the same stream returned, times
 * the stored calibration. The crash test runs this executable as a recording child and kills it.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include <acq/acq_convert.h>
#include <acq/acq_reader.h>
#include <acq/acq_stream.h>
#include <acq/fiff/acq_fiff.h>
#include <acq/fiff/acq_recorder.h>
#include <fiff/fiff_annotations.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#if !defined(_WIN32)
#include <csignal>
#include <sys/resource.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

constexpr double kSfreq = 1000.0;

/** Raw value of channel @p c at stream sample @p s for @p format; exact in double. */
double sampleValue(AcqSampleFormat format, std::uint64_t s, std::size_t c)
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return static_cast<double>(static_cast<std::int64_t>((s * 37 + c * 1009) % 65536) - 32768);
        case AcqSampleFormat::Int24Packed:
            return static_cast<double>(static_cast<std::int64_t>((s * 7919 + c * 104729) % 16777216) - 8388608);
        case AcqSampleFormat::Int32:
            return static_cast<double>(static_cast<std::int32_t>(static_cast<std::uint32_t>(s * 2654435761U + c * 40503U)));
        case AcqSampleFormat::Float32:
            return static_cast<double>(static_cast<float>(static_cast<int>((s * 13 + c * 7) % 2001) - 1000) * 0.001953125f +
                                       static_cast<float>(c));
        default:
            return static_cast<double>((s * 31 + c) % 100003) * 1e-7 - 0.005;
    }
}

/** Writes one sample with relaxed atomic byte stores: the recorder may read a view the producer is overwriting. */
void storeSample(AcqSampleFormat format, std::byte* at, double value)
{
    std::byte bytes[8] = {};
    switch (format) {
        case AcqSampleFormat::Int16: {
            const auto v = static_cast<std::int16_t>(value);
            std::memcpy(bytes, &v, sizeof(v));
            break;
        }
        case AcqSampleFormat::Int24Packed: {
            const auto v = static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
            std::memcpy(bytes, &v, 3); // little-endian hosts
            break;
        }
        case AcqSampleFormat::Int32: {
            const auto v = static_cast<std::int32_t>(value);
            std::memcpy(bytes, &v, sizeof(v));
            break;
        }
        case AcqSampleFormat::Float32: {
            const auto v = static_cast<float>(value);
            std::memcpy(bytes, &v, sizeof(v));
            break;
        }
        default:
            std::memcpy(bytes, &value, sizeof(value));
            break;
    }
    for (std::size_t i = 0; i < bytesPerSample(format); ++i) {
        std::atomic_ref<std::byte>(at[i]).store(bytes[i], std::memory_order_relaxed);
    }
}

//=============================================================================================================

AcqStreamInfo makeInfo(AcqSampleFormat format, AcqLayout layout, std::size_t channels, bool stim)
{
    AcqStreamInfo info;
    info.name = "eeg";
    info.sourceId = "test:42";
    info.format = format;
    info.layout = layout;
    info.sfreq = kSfreq;
    info.clockDomain = "test:42";
    info.eventLabels = {"", "standard", "target"};
    const bool integer = format == AcqSampleFormat::Int16 || format == AcqSampleFormat::Int24Packed ||
        format == AcqSampleFormat::Int32;
    for (std::size_t c = 0; c < channels; ++c) {
        AcqChannelInfo channel;
        channel.name = "EEG " + std::to_string(c + 1);
        channel.kind = AcqChannelKind::Eeg;
        channel.unit = AcqUnit::Volt;
        channel.scale = integer ? 1e-7 : 1.0;
        if (c == 1) {
            channel.scale = 0.5;
            channel.unitExponent = -6;
        }
        info.channels.push_back(channel);
    }
    if (stim) {
        AcqChannelInfo trigger;
        trigger.name = "STI 014";
        trigger.kind = AcqChannelKind::Stim;
        trigger.unit = AcqUnit::None;
        info.channels.push_back(trigger);
    }
    return info;
}

/** Commits samples [first, first + count) of sampleValue() and the events that fall into them. */
[[nodiscard]] bool produce(AcqWriter& writer, AcqSampleFormat format, std::uint64_t first, std::size_t count,
                           const std::vector<AcqEvent>& events = {})
{
    auto view = writer.beginWrite(count);
    if (!view) {
        return false;
    }
    std::size_t done = 0;
    for (std::size_t p = 0; p < view->partCount(); ++p) {
        const AcqWritePart& part = view->part(p);
        for (std::size_t s = 0; s < part.samples; ++s) {
            for (std::size_t c = 0; c < view->channelCount(); ++c) {
                storeSample(format, part.at(c, s), sampleValue(format, first + done + s, c));
            }
        }
        done += part.samples;
    }
    for (const AcqEvent& e : events) {
        if (e.sample >= first && e.sample < first + count && !writer.pushEvent(e).ok()) {
            return false;
        }
    }
    writer.commit(count);
    return true;
}

/** Everything a Lossless reader delivers until the end of the stream, as raw values (samples x channels). */
std::vector<double> drainRaw(AcqReader& reader)
{
    std::vector<double> values;
    for (;;) {
        auto lease = reader.lease(1, 1s);
        if (!lease) {
            break;
        }
        const AcqBlockView& view = **lease;
        std::vector<double> block(view.channelCount() * view.sampleCount());
        AcqConvertOptions raw;
        raw.scaling = AcqScaling::Raw;
        // Column-major channels x samples: one frame after the other.
        const auto frames = AcqMatrixRef<double>::columnMajor(block.data(), view.channelCount(), view.sampleCount());
        if (!decode(view, frames, raw).ok()) {
            break;
        }
        values.insert(values.end(), block.begin(), block.end());
    }
    return values;
}

/** Stim column of the recorder's STI 101 for @p events over samples [first, first + count). */
std::vector<double> stimColumn(const std::vector<AcqEvent>& events, std::uint64_t first, std::size_t count)
{
    std::vector<double> stim(count, 0.0);
    std::int32_t code = 0;
    std::uint64_t end = 0;
    std::size_t next = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint64_t s = first + i;
        while (next < events.size() && events[next].sample == s) {
            code = events[next].code;
            end = s + std::max<std::uint32_t>(events[next].duration, 1U);
            ++next;
        }
        stim[i] = s < end ? static_cast<double>(code) : 0.0;
    }
    return stim;
}

QString python()
{
    QString exe = QStandardPaths::findExecutable(QStringLiteral("python3"));
    return exe.isEmpty() ? QStandardPaths::findExecutable(QStringLiteral("python")) : exe;
}

/** Runs check_recording.py on @p spec; returns "OK", "SKIP ..." or the failure. */
QString checkWithMne(const QTemporaryDir& dir, const QJsonObject& spec)
{
    const QString exe = python();
    if (exe.isEmpty()) {
        return QStringLiteral("SKIP no python");
    }
    const QString specPath = dir.filePath(QStringLiteral("spec.json"));
    QFile out(specPath);
    if (!out.open(QIODevice::WriteOnly)) {
        return QStringLiteral("FAIL cannot write spec");
    }
    out.write(QJsonDocument(spec).toJson());
    out.close();
    QProcess process;
    // Import timings on stderr stand in for notices such as Matplotlib building its font cache.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONPROFILEIMPORTTIME"), QStringLiteral("1"));
    process.setProcessEnvironment(env);
    process.start(exe, {QStringLiteral(ACQ_RECORDER_CHECK_SCRIPT), specPath});
    if (!process.waitForFinished(300000)) {
        return QStringLiteral("FAIL check_recording.py did not finish");
    }
    // Python packages may print notices to stderr (Matplotlib's font cache on a first run).
    const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString text = (output + QLatin1Char('\n') + QString::fromUtf8(process.readAllStandardError())).trimmed();
    if (process.exitCode() == 2) {
        return QStringLiteral("SKIP ") + text;
    }
    return process.exitCode() == 0 && output.endsWith(QStringLiteral("OK")) ? QStringLiteral("OK") : text;
}

bool writeDoubles(const QString& path, const std::vector<double>& values)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const auto bytes = static_cast<qint64>(values.size()) * static_cast<qint64>(sizeof(double));
    return file.write(reinterpret_cast<const char*>(values.data()), bytes) == bytes;
}

QString formatName(AcqSampleFormat format, bool physical)
{
    if (physical) {
        return QStringLiteral("double");
    }
    switch (format) {
        case AcqSampleFormat::Int16:
            return QStringLiteral("short");
        case AcqSampleFormat::Int24Packed:
        case AcqSampleFormat::Int32:
            return QStringLiteral("int");
        case AcqSampleFormat::Float32:
            return QStringLiteral("single");
        default:
            return QStringLiteral("double");
    }
}

/** The recording child of the crash test: records int16 samples until it is killed. */
int recordChild(const char* path)
{
    auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 4, false));
    if (!writer) {
        return 3;
    }
    AcqRecorderOptions options;
    options.bufferSamples = 100;
    auto recorder = AcqRecorder::start(writer->stream(), path, options);
    if (!recorder) {
        std::printf("ERROR %s\n", recorder.error().text().c_str());
        return 4;
    }
    std::uint64_t next = 0;
    std::uint64_t reported = 0;
    for (;;) {
        if (!produce(*writer, AcqSampleFormat::Int16, next, 50, {{next + 10, 0, 1}})) {
            return 5;
        }
        next += 50;
        const std::uint64_t flushed = recorder->stats().samples;
        if (flushed != reported) {
            reported = flushed;
            std::printf("flushed %llu\n", static_cast<unsigned long long>(flushed));
            std::fflush(stdout);
        }
        std::this_thread::sleep_for(2ms);
    }
}

} // namespace

//=============================================================================================================
/**
 * Tests the FIFF bridge of mne_acq.
 */
class TestAcqRecorder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void convertsFiffInfo();
    void refusesInfoWithoutFiff();
    void recordsNativeFormats_data();
    void recordsNativeFormats();
    void recordsLostSamplesAsSkips();
    void splitsFiles();
    void recordsBids();
    void survivesSigkill();
    void reportsErrors();
    void reportsDiskFailure();
    void lifeCycle();

private:
    struct Recorded
    {
        std::vector<double> stored; // samples x channels as stored, stim included
        std::vector<AcqEvent> events;
        AcqRecorderStats stats;
        StringList files;
        QStringList chNames;
        AcqSampleFormat format = AcqSampleFormat::Float32;
        bool physical = false;
    };

    Recorded record(const AcqStreamInfo& info, const std::string& path, std::size_t samples,
                    const AcqRecorderOptions& options, std::uint64_t skipFirst = 0);
    QJsonObject spec(const Recorded& r, const QString& fif, std::size_t channels) const;

    QTemporaryDir m_dir;
};

//=============================================================================================================

void TestAcqRecorder::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

//=============================================================================================================

TestAcqRecorder::Recorded TestAcqRecorder::record(const AcqStreamInfo& info, const std::string& path,
                                                  std::size_t samples, const AcqRecorderOptions& options,
                                                  std::uint64_t skipFirst)
{
    Recorded r;
    r.format = info.format;
    auto writer = AcqWriter::create(info);
    if (!writer) {
        Log::warning("{}", QString::fromStdString(writer.error().text()).toStdString());
        return r;
    }
    // Samples before the recording starts: the file then starts at FIFF first_samp = skipFirst.
    if (skipFirst > 0) {
        if (!produce(*writer, info.format, 0, static_cast<std::size_t>(skipFirst))) {
            return r;
        }
    }
    auto reference = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto recorder = AcqRecorder::start(writer->stream(), path, options);
    if (!recorder) {
        Log::warning("{}", QString::fromStdString(recorder.error().text()).toStdString());
        return r;
    }
    for (std::uint64_t s = skipFirst; s < skipFirst + samples; s += 97) {
        const auto k = static_cast<std::int32_t>((s / 97) % 3);
        r.events.push_back({s + 3, k == 0 ? 0U : 5U, k == 2 ? 7 : k + 1});
    }
    std::uint64_t next = skipFirst;
    std::size_t step = 1;
    while (next < skipFirst + samples) {
        const std::size_t n = std::min<std::size_t>(step, skipFirst + samples - next);
        if (!produce(*writer, info.format, next, n, r.events)) {
            return r;
        }
        next += n;
        step = step * 3 % 401 + 1;
    }
    writer->finish();
    const std::vector<double> raw = drainRaw(*reference);
    const AcqError stopped = recorder->stop();
    if (!stopped.ok()) {
        Log::warning("{}", QString::fromStdString(stopped.text()).toStdString());
        return r;
    }
    r.stats = recorder->stats();
    r.files = recorder->files();
    r.physical = std::any_of(info.channels.begin(), info.channels.end(),
                             [](const AcqChannelInfo& c) { return c.offset != 0.0 || c.digitalBits != 0; });
    const bool addStim = std::none_of(info.channels.begin(), info.channels.end(),
                                      [](const AcqChannelInfo& c) { return c.kind == AcqChannelKind::Stim; });
    const std::size_t channels = info.channelCount();
    const std::size_t out = channels + (addStim ? 1 : 0);
    const std::vector<double> stim = stimColumn(r.events, skipFirst, samples);
    r.stored.resize(samples * out);
    for (std::size_t s = 0; s < samples; ++s) {
        for (std::size_t c = 0; c < channels; ++c) {
            double value = raw[s * channels + c];
            if (r.physical) {
                // Stored as raw * scale + offset in channel units, with the power of ten as calibration.
                value *= info.channels[c].scale;
                value += info.channels[c].offset;
            }
            r.stored[s * out + c] = value;
        }
        if (addStim) {
            r.stored[s * out + channels] = stim[s];
        }
    }
    for (const AcqChannelInfo& c : info.channels) {
        r.chNames << QString::fromStdString(c.name);
    }
    if (addStim) {
        r.chNames << QStringLiteral("STI 101");
    }
    return r;
}

//=============================================================================================================

QJsonObject TestAcqRecorder::spec(const Recorded& r, const QString& fif, std::size_t channels) const
{
    // Next to the test's files, never inside a BIDS dataset, where the validator would reject it.
    const QString expected = m_dir.filePath(QFileInfo(fif).fileName() + QStringLiteral(".expected"));
    writeDoubles(expected, r.stored);
    QJsonObject spec;
    spec[QStringLiteral("fif")] = fif;
    spec[QStringLiteral("expected")] = expected;
    spec[QStringLiteral("first_samp")] = static_cast<qint64>(r.stats.firstSample);
    spec[QStringLiteral("n_times")] = static_cast<qint64>(r.stored.size() / channels);
    spec[QStringLiteral("orig_format")] = formatName(r.format, r.physical);
    spec[QStringLiteral("files")] = static_cast<int>(r.files.size());
    spec[QStringLiteral("ch_names")] = QJsonArray::fromStringList(r.chNames);
    FiffRawData raw(QString::fromStdString(r.files.front()).toStdString());
    spec[QStringLiteral("meas_date")] = QJsonArray{raw.info.meas_date[0], raw.info.meas_date[1]};
    spec[QStringLiteral("device_info")] = QJsonObject{{QStringLiteral("type"), QStringLiteral("test")},
                                                      {QStringLiteral("serial"), QStringLiteral("test:42")}};
    spec[QStringLiteral("description_contains")] = QStringLiteral("clock_domain=test:42");
    QJsonArray annotations;
    const char* labels[] = {"", "standard", "target"};
    for (const AcqEvent& e : r.events) {
        annotations.append(QJsonArray{static_cast<qint64>(e.sample), static_cast<qint64>(e.duration),
                                      e.code < 3 ? QString::fromLatin1(labels[e.code]) : QString::number(e.code)});
    }
    spec[QStringLiteral("annotations")] = annotations;
    return spec;
}

//=============================================================================================================

void TestAcqRecorder::convertsFiffInfo()
{
    // From FIFF and back: every channel field and the measurement info survive.
    FiffInfo fiff;
    fiff.sfreq = 600.0f;
    fiff.linefreq = 50.0f;
    fiff.highpass = 0.1f;
    fiff.lowpass = 172.0f;
    fiff.experimenter = "cd";
    fiff.meas_date[0] = 1793700000;
    fiff.meas_date[1] = 5;
    fiff.bads.push_back("MEG 0113");
    const int kinds[] = {FIFFV_MEG_CH, FIFFV_EEG_CH, FIFFV_STIM_CH, FIFFV_RESP_CH, FIFFV_BIO_CH};
    for (int k = 0; k < 5; ++k) {
        FiffChInfo ch;
        ch.ch_name = k == 0 ? std::string("MEG 0113") : "CH " + std::to_string(k);
        ch.kind = kinds[k];
        ch.scanNo = k + 1;
        ch.logNo = 100 + k;
        ch.unit = k == 0 ? FIFF_UNIT_T : FIFF_UNIT_V;
        ch.unit_mul = k == 1 ? -6 : 0;
        ch.cal = 3e-13f * static_cast<float>(k + 1);
        ch.range = k == 2 ? 2.0f : 1.0f;
        ch.chpos.coil_type = k == 0 ? 3012 : 1;
        ch.chpos.r0 = Eigen::Vector3f(0.01f * static_cast<float>(k), 0.02f, 0.03f);
        ch.coord_frame = FIFFV_COORD_DEVICE;
        fiff.chs.push_back(ch);
        fiff.ch_names.push_back(ch.ch_name);
    }
    fiff.nchan = 5;
    const AcqStreamInfo info = fromFiffInfo(fiff, AcqSampleFormat::Int32);
    QCOMPARE(info.format, AcqSampleFormat::Int32);
    QCOMPARE(info.channelCount(), std::size_t(5));
    QCOMPARE(info.channels[0].kind, AcqChannelKind::Meg);
    QCOMPARE(info.channels[2].kind, AcqChannelKind::Stim);
    QCOMPARE(info.channels[3].kind, AcqChannelKind::Respiration);
    QCOMPARE(info.channels[4].kind, AcqChannelKind::Unknown);
    QCOMPARE(info.channels[1].unitExponent, -6);
    QCOMPARE(info.channels[2].scale, static_cast<double>(fiff.chs[2].cal) * 2.0);
    QCOMPARE(info.extensions.size(), std::size_t(1));

    auto back = toFiffInfo(info);
    QVERIFY(back);
    QCOMPARE(back->nchan, 5);
    QCOMPARE(back->sfreq, 600.0f);
    QCOMPARE(back->linefreq, 50.0f);
    QCOMPARE(back->experimenter, fiff.experimenter);
    QCOMPARE(back->meas_date[0], fiff.meas_date[0]);
    QCOMPARE(back->bads, fiff.bads);
    for (std::size_t k = 0; k < 5; ++k) {
        QCOMPARE(back->chs[k].ch_name, fiff.chs[k].ch_name);
        QCOMPARE(back->chs[k].kind, fiff.chs[k].kind);
        QCOMPARE(back->chs[k].logNo, fiff.chs[k].logNo);
        QCOMPARE(back->chs[k].unit, fiff.chs[k].unit);
        QCOMPARE(back->chs[k].unit_mul, fiff.chs[k].unit_mul);
        QCOMPARE(back->chs[k].cal, fiff.chs[k].cal);
        QCOMPARE(back->chs[k].range, fiff.chs[k].range);
        QCOMPARE(back->chs[k].chpos.coil_type, fiff.chs[k].chpos.coil_type);
        QCOMPARE(back->chs[k].chpos.r0, fiff.chs[k].chpos.r0);
        QCOMPARE(back->chs[k].coord_frame, fiff.chs[k].coord_frame);
    }

    // An edited channel wins over its FIFF fields: the new scale with its power of ten goes into cal.
    AcqStreamInfo edited = info;
    edited.channels[1].scale = 0.25;
    edited.channels[1].unitExponent = -3;
    auto changed = toFiffInfo(edited);
    QVERIFY(changed);
    QCOMPARE(changed->chs[1].cal, 0.25e-3f);
    QCOMPARE(changed->chs[1].range, 1.0f);
    QCOMPARE(changed->chs[1].unit_mul, 0);

    // A damaged payload is reported rather than half applied.
    AcqStreamInfo corrupt = info;
    corrupt.extensions[0].data.resize(10);
    QCOMPARE(toFiffInfo(corrupt).error().code(), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqRecorder::refusesInfoWithoutFiff()
{
    // Channels described only by kind and unit get FIFF fields from them; Unknown units become misc.
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 2, true);
    AcqChannelInfo hbo;
    hbo.name = "S1_D1 hbo";
    hbo.kind = AcqChannelKind::Fnirs;
    hbo.unit = AcqUnit::Molar;
    info.channels.push_back(hbo);
    hbo.name = "S1_D1 hbr";
    info.channels.push_back(hbo);
    AcqChannelInfo unknown;
    unknown.name = "AUX";
    unknown.kind = AcqChannelKind::Ecg;
    info.channels.push_back(unknown);
    AcqChannelInfo marker;
    marker.name = "MARKER";
    marker.kind = AcqChannelKind::Marker;
    marker.unit = AcqUnit::None;
    info.channels.push_back(marker);
    auto fiff = toFiffInfo(info);
    QVERIFY(fiff);
    QCOMPARE(fiff->chs[0].kind, FIFFV_EEG_CH);
    QCOMPARE(fiff->chs[0].chpos.coil_type, FIFFV_COIL_EEG);
    QCOMPARE(fiff->chs[0].unit, FIFF_UNIT_V);
    QVERIFY(qFuzzyCompare(fiff->chs[1].cal, 0.5e-6f));
    QCOMPARE(fiff->chs[2].kind, FIFFV_STIM_CH);
    QCOMPARE(fiff->chs[2].unit, FIFF_UNIT_NONE);
    QCOMPARE(fiff->chs[3].chpos.coil_type, FIFFV_COIL_FNIRS_HBO);
    QCOMPARE(fiff->chs[4].chpos.coil_type, FIFFV_COIL_FNIRS_HBR);
    QCOMPARE(fiff->chs[4].unit, FIFF_UNIT_MOL);
    QCOMPARE(fiff->chs[5].kind, FIFFV_MISC_CH);
    QCOMPARE(fiff->chs[5].unit, FIFF_UNIT_NONE);
    QCOMPARE(fiff->chs[6].kind, FIFFV_STIM_CH); // a device marker row is a trigger channel
    QCOMPARE(fiff->lowpass, 500.0f);

    // FIFF has no complex or segmented raw data.
    AcqStreamInfo complex = info;
    complex.format = AcqSampleFormat::ComplexInt16;
    QCOMPARE(toFiffInfo(complex).error().code(), AcqErrorCode::UnsupportedFormat);
    AcqStreamInfo segmented = info;
    segmented.segmentation.mode = AcqSegmentation::Mode::Triggered;
    QCOMPARE(toFiffInfo(segmented).error().code(), AcqErrorCode::UnsupportedFormat);
}

//=============================================================================================================

void TestAcqRecorder::recordsNativeFormats_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("layout");
    QTest::addColumn<bool>("stim");
    QTest::addColumn<double>("offset");
    QTest::addColumn<bool>("direct");
    QTest::addColumn<quint64>("first");
    QTest::addColumn<int>("digital");

    const int il = static_cast<int>(AcqLayout::Interleaved);
    QTest::newRow("int16") << static_cast<int>(AcqSampleFormat::Int16) << il << false << 0.0 << true << quint64(0) << 0;
    QTest::newRow("int16 own stim") << static_cast<int>(AcqSampleFormat::Int16) << il << true << 0.0 << true << quint64(0) << 0;
    QTest::newRow("int24") << static_cast<int>(AcqSampleFormat::Int24Packed) << il << false << 0.0 << true << quint64(0) << 0;
    QTest::newRow("int32") << static_cast<int>(AcqSampleFormat::Int32) << il << false << 0.0 << false << quint64(0) << 0;
    QTest::newRow("float32") << static_cast<int>(AcqSampleFormat::Float32) << il << false << 0.0 << true << quint64(0) << 0;
    QTest::newRow("float32 planar") << static_cast<int>(AcqSampleFormat::Float32) << static_cast<int>(AcqLayout::Planar)
                                    << false << 0.0 << true << quint64(0) << 0;
    QTest::newRow("float64 late start") << static_cast<int>(AcqSampleFormat::Float64) << il << false << 0.0 << true
                                        << quint64(1234) << 0;
    QTest::newRow("int32 digital bits (physical)") << static_cast<int>(AcqSampleFormat::Int32) << il << false << 0.0
                                                   << true << quint64(0) << 3;
    QTest::newRow("int16 offset (physical)") << static_cast<int>(AcqSampleFormat::Int16) << il << false << 0.5 << true
                                             << quint64(0) << 0;
}

void TestAcqRecorder::recordsNativeFormats()
{
    QFETCH(int, format);
    QFETCH(int, layout);
    QFETCH(bool, stim);
    QFETCH(double, offset);
    QFETCH(bool, direct);
    QFETCH(quint64, first);
    QFETCH(int, digital);
    const auto fmt = static_cast<AcqSampleFormat>(format);
    AcqStreamInfo info = makeInfo(fmt, static_cast<AcqLayout>(layout), 3, stim);
    info.channels[2].offset = offset;
    info.channels[0].digitalBits = static_cast<std::uint8_t>(digital);

    const QString path = m_dir.filePath(QStringLiteral("%1_raw.fif").arg(QTest::currentDataTag()).replace(' ', '_'));
    AcqRecorderOptions options;
    options.bufferSamples = 1000;
    options.directIo = direct;
    options.device = {"test", "", "Test amplifier", "test://42"};
    const std::size_t samples = 2500;
    const Recorded r = record(info, path.toStdString(), samples, options, first);
    QCOMPARE(r.files.size(), std::size_t(1));
    QCOMPARE(r.stats.firstSample, first);
    QCOMPARE(r.stats.samples, std::uint64_t(samples));
    QCOMPARE(r.stats.lost, std::uint64_t(0));
    QCOMPARE(r.stats.events, std::uint64_t(r.events.size()));
    QCOMPARE(r.stats.buffers, std::uint64_t(3));
    QCOMPARE(r.stats.files, std::size_t(1));
#if defined(__APPLE__)
    QCOMPARE(r.stats.directIo, direct);
#endif

    // MNE-CPP reads raw * cal * range of every stored sample.
    FiffRawData raw(path.toStdString());
    QCOMPARE(raw.first_samp, static_cast<fiff_int_t>(first));
    QCOMPARE(raw.last_samp, static_cast<fiff_int_t>(first + samples - 1));
    QCOMPARE(raw.info.device_info.type, QStringLiteral("test"));
    QCOMPARE(raw.info.device_info.model, QStringLiteral("Test amplifier"));
    QCOMPARE(raw.info.device_info.serial, QStringLiteral("test:42"));
    QVERIFY((raw.info.description.find("lock=Free") != std::string::npos));
    QVERIFY(raw.info.meas_date[0] > 1700000000);
    Eigen::MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    const std::size_t out = static_cast<std::size_t>(raw.info.nchan);
    QCOMPARE(out, static_cast<std::size_t>(r.chNames.size()));
    for (std::size_t s = 0; s < samples; ++s) {
        for (std::size_t c = 0; c < out; ++c) {
            const double expected = r.stored[s * out + c] * raw.cals(static_cast<Eigen::Index>(c));
            if (data(static_cast<Eigen::Index>(c), static_cast<Eigen::Index>(s)) != expected) {
                QFAIL(qPrintable(QStringLiteral("sample %1 channel %2: %3 != %4")
                                     .arg(s)
                                     .arg(c)
                                     .arg(data(static_cast<Eigen::Index>(c), static_cast<Eigen::Index>(s)))
                                     .arg(expected)));
            }
        }
    }
    FiffStream stream(path.toStdU16String());
    QVERIFY(stream.open());
    FiffAnnotations annotations;
    fiff_int_t origTime[2] = {0, 0};
    QVERIFY(stream.read_annotations(stream.dirtree(), annotations, origTime));
    QCOMPARE(origTime[0], raw.info.meas_date[0]);
    QCOMPARE(annotations.size(), static_cast<int>(r.events.size()));
    QCOMPARE(annotations[1].description, QStringLiteral("target"));

    const QString result = checkWithMne(m_dir, spec(r, path, out));
    QVERIFY2(result == QStringLiteral("OK") || result.startsWith(QStringLiteral("SKIP")), qPrintable(result));
    if (result != QStringLiteral("OK")) {
        Log::warning("{}", result.toStdString());
    }
}

//=============================================================================================================

void TestAcqRecorder::recordsLostSamplesAsSkips()
{
    // A tiny ring and a producer that never waits: the recorder's reader falls behind and loses samples.
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Int32, AcqLayout::Interleaved, 8, false);
    AcqStreamOptions ring;
    ring.capacitySamples = 64;
    ring.mirror = false;
    auto writer = AcqWriter::create(info, ring);
    QVERIFY(writer);
    const QString path = m_dir.filePath(QStringLiteral("lossy_raw.fif"));
    AcqRecorderOptions options;
    options.bufferSamples = 512;
    auto recorder = AcqRecorder::start(writer->stream(), path.toStdString(), options);
    QVERIFY(recorder);
    const std::size_t samples = 400000;
    for (std::uint64_t s = 0; s < samples; s += 32) {
        QVERIFY(produce(*writer, info.format, s, 32));
    }
    writer->finish();
    QVERIFY(recorder->stop().ok());
    const AcqRecorderStats stats = recorder->stats();
    QVERIFY2(stats.lost > 0, "the reader was expected to fall behind a 64-sample ring");
    QCOMPARE(stats.samples, std::uint64_t(samples));

    // Every sample is either the produced value or a zero inside a BAD_ACQ_SKIP annotation.
    FiffRawData raw(path.toStdString());
    Eigen::MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    QCOMPARE(data.cols(), static_cast<Eigen::Index>(samples));
    FiffStream stream(path.toStdU16String());
    QVERIFY(stream.open());
    FiffAnnotations annotations;
    fiff_int_t origTime[2] = {0, 0};
    QVERIFY(stream.read_annotations(stream.dirtree(), annotations, origTime));
    std::vector<bool> skipped(samples, false);
    std::uint64_t total = 0;
    QJsonArray expectedAnnotations;
    for (std::size_t i = 0; i < annotations.size(); ++i) {
        QCOMPARE(annotations[i].description, std::string("BAD_ACQ_SKIP"));
        const auto first = static_cast<std::uint64_t>(std::llround(annotations[i].onset * kSfreq));
        const auto count = static_cast<std::uint64_t>(std::llround(annotations[i].duration * kSfreq));
        total += count;
        for (std::uint64_t s = first; s < first + count; ++s) {
            skipped[s] = true;
        }
        expectedAnnotations.append(QJsonArray{static_cast<qint64>(first), static_cast<qint64>(count), "BAD_ACQ_SKIP"});
    }
    QCOMPARE(total, stats.lost);
    std::vector<double> stored(samples * 9, 0.0);
    for (std::size_t s = 0; s < samples; ++s) {
        for (std::size_t c = 0; c < 8; ++c) {
            const double value = skipped[s] ? 0.0 : sampleValue(info.format, s, c);
            stored[s * 9 + c] = value;
            if (data(static_cast<Eigen::Index>(c), static_cast<Eigen::Index>(s)) !=
                value * raw.cals(static_cast<Eigen::Index>(c))) {
                QFAIL(qPrintable(QStringLiteral("sample %1 channel %2").arg(s).arg(c)));
            }
        }
    }

    Recorded r;
    r.stored = stored;
    r.stats = stats;
    r.files = recorder->files();
    r.format = info.format;
    for (std::size_t c = 0; c < 8; ++c) {
        r.chNames << QString::fromStdString(info.channels[c].name);
    }
    r.chNames << QStringLiteral("STI 101");
    QJsonObject check = spec(r, path, 9);
    check[QStringLiteral("annotations")] = expectedAnnotations;
    check[QStringLiteral("device_info")] = QJsonObject{{QStringLiteral("serial"), QStringLiteral("test:42")}};
    const QString result = checkWithMne(m_dir, check);
    QVERIFY2(result == QStringLiteral("OK") || result.startsWith(QStringLiteral("SKIP")), qPrintable(result));
    if (result != QStringLiteral("OK")) {
        Log::warning("{}", result.toStdString());
    }
}

//=============================================================================================================

void TestAcqRecorder::splitsFiles()
{
    // 2 MiB parts of 16 float64 channels: the first keeps 1 MiB free for its annotations, as MNE-Python.
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Float64, AcqLayout::Interleaved, 16, true);
    // The doubled separator checks that later parts keep the directory exactly as given.
    const QString path = m_dir.path() + QStringLiteral("//split_raw.fif");
    AcqRecorderOptions options;
    options.bufferSamples = 500;
    options.splitBytes = 2 * 1048576;
    options.device = {"test", "", "", ""};
    const std::size_t samples = 40000;
    const Recorded r = record(info, path.toStdString(), samples, options);
    QVERIFY(r.files.size() >= 3);
    QCOMPARE(r.stats.files, r.files.size());
    QCOMPARE(QString::fromStdString(r.files[0]), path);
    QCOMPARE(QString::fromStdString(r.files[1]), m_dir.path() + QStringLiteral("//split_raw-1.fif"));
    std::uint64_t next = 0;
    for (const std::string& part : r.files) {
        QVERIFY(QFileInfo(QString::fromStdString(part)).size() <= static_cast<qint64>(options.splitBytes));
        FiffRawData raw(QString::fromStdString(part).toStdString());
        QCOMPARE(raw.first_samp, static_cast<fiff_int_t>(next));
        next = static_cast<std::uint64_t>(raw.last_samp) + 1;
    }
    QCOMPARE(next, std::uint64_t(samples));

    const QString result = checkWithMne(m_dir, spec(r, path, 17));
    QVERIFY2(result == QStringLiteral("OK") || result.startsWith(QStringLiteral("SKIP")), qPrintable(result));
    if (result != QStringLiteral("OK")) {
        Log::warning("{}", result.toStdString());
    }
}

//=============================================================================================================

void TestAcqRecorder::recordsBids()
{
    const QString root = m_dir.filePath(QStringLiteral("bids"));
    AcqBidsPath bids{root.toStdString(), "01", "1", "oddball", "", "Recorder test"};
    AcqStreamInfo info = makeInfo(AcqSampleFormat::Int16, AcqLayout::Interleaved, 4, false);
    AcqRecorderOptions options;
    options.bufferSamples = 1000;
    options.device = {"test", "", "Test amplifier", "test://42"};

    auto writer = AcqWriter::create(info);
    QVERIFY(writer);
    auto reference = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto recorder = AcqRecorder::start(writer->stream(), bids, options);
    QVERIFY(recorder);
    std::vector<AcqEvent> events = {{100, 0, 1}, {1100, 0, 2}, {2100, 0, 1}};
    QVERIFY(produce(*writer, info.format, 0, 3000, events));
    writer->finish();
    const std::vector<double> rawValues = drainRaw(*reference);
    QVERIFY(recorder->stop().ok());

    const QString fif = root + QStringLiteral("/sub-01/ses-1/meg/sub-01_ses-1_task-oddball_meg.fif");
    QCOMPARE(QDir::fromNativeSeparators(QString::fromStdString(recorder->files().front())), fif); // Windows joins with '\\'
    for (const char* sidecar : {"/dataset_description.json", "/sub-01/ses-1/meg/sub-01_ses-1_task-oddball_meg.json",
                                "/sub-01/ses-1/meg/sub-01_ses-1_task-oddball_channels.tsv",
                                "/sub-01/ses-1/meg/sub-01_ses-1_task-oddball_events.tsv"}) {
        QVERIFY2(QFileInfo::exists(root + QString::fromLatin1(sidecar)), sidecar);
    }

    Recorded r;
    r.format = info.format;
    r.stats = recorder->stats();
    r.files = recorder->files();
    r.events = events;
    const std::vector<double> stim = stimColumn(events, 0, 3000);
    r.stored.resize(3000 * 5);
    for (std::size_t s = 0; s < 3000; ++s) {
        for (std::size_t c = 0; c < 4; ++c) {
            r.stored[s * 5 + c] = rawValues[s * 4 + c];
        }
        r.stored[s * 5 + 4] = stim[s];
        if (s < 4) {
            r.chNames << QString::fromStdString(info.channels[s].name);
        }
    }
    r.chNames << QStringLiteral("STI 101");
    QJsonObject check = spec(r, fif, 5);
    check[QStringLiteral("bids")] =
        QJsonObject{{QStringLiteral("root"), root}, {QStringLiteral("subject"), QStringLiteral("01")}, {QStringLiteral("session"), QStringLiteral("1")}, {QStringLiteral("task"), QStringLiteral("oddball")}};
    const QString result = checkWithMne(m_dir, check);
    QVERIFY2(result == QStringLiteral("OK") || result.startsWith(QStringLiteral("SKIP")), qPrintable(result));
    if (result != QStringLiteral("OK")) {
        Log::warning("{}", result.toStdString());
    }

    // A split BIDS recording names every part split-NN, the first renamed once the second exists.
    const QString splitRoot = m_dir.filePath(QStringLiteral("bids_split"));
    AcqBidsPath split{splitRoot.toStdString(), "02", "", "rest", "1", ""};
    AcqStreamInfo wide = makeInfo(AcqSampleFormat::Float64, AcqLayout::Interleaved, 32, false);
    options.splitBytes = 2 * 1048576;
    options.bufferSamples = 200;
    auto splitWriter = AcqWriter::create(wide);
    QVERIFY(splitWriter);
    auto splitRecorder = AcqRecorder::start(splitWriter->stream(), split, options);
    QVERIFY(splitRecorder);
    for (std::uint64_t s = 0; s < 12000; s += 1000) {
        QVERIFY(produce(*splitWriter, wide.format, s, 1000));
    }
    splitWriter->finish();
    QVERIFY(splitRecorder->stop().ok());
    const auto files = splitRecorder->files();
    QVERIFY(files.size() >= 2);
    QCOMPARE(QDir::fromNativeSeparators(QString::fromStdString(files[0])),
             splitRoot + QStringLiteral("/sub-02/meg/sub-02_task-rest_run-01_split-01_meg.fif"));
    QCOMPARE(QDir::fromNativeSeparators(QString::fromStdString(files[1])),
             splitRoot + QStringLiteral("/sub-02/meg/sub-02_task-rest_run-01_split-02_meg.fif"));
    QVERIFY(!QFileInfo::exists(splitRoot + QStringLiteral("/sub-02/meg/sub-02_task-rest_run-01_meg.fif")));
}

//=============================================================================================================

void TestAcqRecorder::survivesSigkill()
{
    const QString path = m_dir.filePath(QStringLiteral("killed_raw.fif"));
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--record-child"), path});
    QVERIFY(child.waitForStarted());
    std::uint64_t flushed = 0;
    const auto deadline = std::chrono::steady_clock::now() + 60s;
    while (flushed < 2000 && std::chrono::steady_clock::now() < deadline) {
        if (child.waitForReadyRead(1000)) {
            while (child.canReadLine()) {
                const QByteArray line = child.readLine().trimmed();
                if (line.startsWith("flushed ")) {
                    flushed = line.mid(8).toULongLong();
                }
            }
        }
        QVERIFY2(child.state() == QProcess::Running, child.readAllStandardError().constData());
    }
    QVERIFY(flushed >= 2000);
    child.kill(); // SIGKILL on POSIX, TerminateProcess on Windows
    QVERIFY(child.waitForFinished(10000));

    // Everything reported as flushed is there, in whole buffers, and identical to what was produced.
    FiffRawData raw(path.toStdString());
    QCOMPARE(raw.first_samp, 0);
    const auto samples = static_cast<std::uint64_t>(raw.last_samp) + 1;
    QVERIFY2(samples >= flushed, qPrintable(QStringLiteral("%1 < %2").arg(samples).arg(flushed)));
    QCOMPARE(samples % 100, std::uint64_t(0));
    Eigen::MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    Recorded r;
    r.format = AcqSampleFormat::Int16;
    r.files = {path.toStdString()};
    const std::size_t n = static_cast<std::size_t>(samples);
    std::vector<AcqEvent> events;
    for (std::uint64_t s = 0; s < samples; s += 50) {
        events.push_back({s + 10, 0, 1});
    }
    const std::vector<double> stim = stimColumn(events, 0, n);
    r.stored.resize(n * 5);
    for (std::size_t s = 0; s < n; ++s) {
        for (std::size_t c = 0; c < 4; ++c) {
            r.stored[s * 5 + c] = sampleValue(AcqSampleFormat::Int16, s, c);
            if (data(static_cast<Eigen::Index>(c), static_cast<Eigen::Index>(s)) !=
                r.stored[s * 5 + c] * raw.cals(static_cast<Eigen::Index>(c))) {
                QFAIL(qPrintable(QStringLiteral("sample %1 channel %2").arg(s).arg(c)));
            }
        }
        r.stored[s * 5 + 4] = stim[s];
        QCOMPARE(data(4, static_cast<Eigen::Index>(s)), stim[s]);
    }
    for (int c = 0; c < 4; ++c) {
        r.chNames << QStringLiteral("EEG %1").arg(c + 1);
    }
    r.chNames << QStringLiteral("STI 101");
    // The annotations are written on stop(), which the killed writer never reached.
    QJsonObject check = spec(r, path, 5);
    check[QStringLiteral("annotations")] = QJsonArray();
    check[QStringLiteral("device_info")] = QJsonObject{{QStringLiteral("serial"), QStringLiteral("test:42")}};
    const QString result = checkWithMne(m_dir, check);
    QVERIFY2(result == QStringLiteral("OK") || result.startsWith(QStringLiteral("SKIP")), qPrintable(result));
    if (result != QStringLiteral("OK")) {
        Log::warning("{}", result.toStdString());
    }
}

//=============================================================================================================

void TestAcqRecorder::reportsErrors()
{
    auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 2, false));
    QVERIFY(writer);
    const std::string path = m_dir.filePath(QStringLiteral("errors_raw.fif")).toStdString();

    AcqRecorderOptions options;
    options.splitBytes = 2147483649ULL;
    QCOMPARE(AcqRecorder::start(writer->stream(), path, options).error().code(), AcqErrorCode::InvalidParameter);
    options.splitBytes = 65536;
    QCOMPARE(AcqRecorder::start(writer->stream(), path, options).error().code(), AcqErrorCode::InvalidParameter);
    options = {};
    options.bufferSamples = std::size_t(1) << 30U;
    QCOMPARE(AcqRecorder::start(writer->stream(), path, options).error().code(), AcqErrorCode::InvalidParameter);

    // A directory where the file should be.
    QCOMPARE(AcqRecorder::start(writer->stream(), m_dir.path().toStdString()).error().code(), AcqErrorCode::DeviceIo);
    QCOMPARE(AcqRecorder::start(AcqStream(), path).error().code(), AcqErrorCode::InvalidState);

    AcqStreamInfo complex = makeInfo(AcqSampleFormat::ComplexFloat32, AcqLayout::Interleaved, 2, false);
    auto complexWriter = AcqWriter::create(complex);
    QVERIFY(complexWriter);
    QCOMPARE(AcqRecorder::start(complexWriter->stream(), path).error().code(), AcqErrorCode::UnsupportedFormat);
    AcqStreamInfo irregular = makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 2, false);
    irregular.sfreq = 0.0;
    auto irregularWriter = AcqWriter::create(irregular);
    QVERIFY(irregularWriter);
    QCOMPARE(AcqRecorder::start(irregularWriter->stream(), path).error().code(), AcqErrorCode::UnsupportedFormat);

    AcqStreamOptions oneReader;
    oneReader.maxReaders = 1;
    auto full = AcqWriter::create(makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 2, false), oneReader);
    QVERIFY(full);
    auto taken = full->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(taken);
    QCOMPARE(AcqRecorder::start(full->stream(), path).error().code(), AcqErrorCode::TooManyReaders);

    // BIDS labels are alphanumeric, and root, subject and task are required.
    const std::string root = m_dir.filePath(QStringLiteral("bad_bids")).toStdString();
    for (const AcqBidsPath& bad : {AcqBidsPath{"", "01", "", "x", "", ""}, AcqBidsPath{root, "", "", "x", "", ""},
                                   AcqBidsPath{root, "0_1", "", "x", "", ""}, AcqBidsPath{root, "01", "", "", "", ""},
                                   AcqBidsPath{root, "01", "a b", "x", "", ""}, AcqBidsPath{root, "01", "", "x", "1-2", ""}}) {
        QCOMPARE(AcqRecorder::start(writer->stream(), bad).error().code(), AcqErrorCode::InvalidParameter);
    }

    // A thread policy the system refuses fails start().
    options = {};
    options.thread.cpus = {100000};
    const AcqError refused = AcqRecorder::start(writer->stream(), path, options).error();
    QVERIFY(refused.code() == AcqErrorCode::InvalidParameter || refused.code() == AcqErrorCode::PlatformUnsupported);
}

//=============================================================================================================

void TestAcqRecorder::reportsDiskFailure()
{
#if defined(_WIN32)
    QSKIP("RLIMIT_FSIZE is POSIX");
#else
    // A file size limit makes the writes fail with EFBIG; stop() reports it.
    auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Float64, AcqLayout::Interleaved, 8, false));
    QVERIFY(writer);
    const QString path = m_dir.filePath(QStringLiteral("full_raw.fif"));
    AcqRecorderOptions options;
    options.bufferSamples = 100;
    auto recorder = AcqRecorder::start(writer->stream(), path.toStdString(), options);
    QVERIFY(recorder);
    rlimit original{};
    QCOMPARE(getrlimit(RLIMIT_FSIZE, &original), 0);
    const auto previous = std::signal(SIGXFSZ, SIG_IGN);
    rlimit limited = original;
    limited.rlim_cur = 64 * 1024;
    QCOMPARE(setrlimit(RLIMIT_FSIZE, &limited), 0);
    for (std::uint64_t s = 0; s < 4000; s += 100) {
        QVERIFY(produce(*writer, AcqSampleFormat::Float64, s, 100));
    }
    writer->finish();
    const AcqError error = recorder->stop();
    QCOMPARE(setrlimit(RLIMIT_FSIZE, &original), 0);
    std::signal(SIGXFSZ, previous);
    QCOMPARE(error.code(), AcqErrorCode::DeviceIo);
    QVERIFY(!recorder->isRecording());
    QCOMPARE(recorder->stop().code(), AcqErrorCode::DeviceIo);

    // What reached the disk is still readable.
    FiffRawData raw(path.toStdString());
    QVERIFY(raw.last_samp >= 0);
#endif
}

//=============================================================================================================

void TestAcqRecorder::lifeCycle()
{
    AcqRecorder empty;
    QVERIFY(!empty.isRecording());
    QVERIFY(empty.stop().ok());
    QVERIFY(empty.files().empty());
    QCOMPARE(empty.stats().samples, std::uint64_t(0));

    auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Float32, AcqLayout::Interleaved, 2, false));
    QVERIFY(writer);
    const QString path = m_dir.filePath(QStringLiteral("nested/dir/life_raw.fif"));
    auto started = AcqRecorder::start(writer->stream(), path.toStdString());
    QVERIFY(started);
    AcqRecorder recorder = std::move(*started);
    QVERIFY(recorder.isRecording());
    QCOMPARE(recorder.files().size(), std::size_t(1));
    QVERIFY(produce(*writer, AcqSampleFormat::Float32, 0, 1500));

    // Moving onto a recording stops the old one; stop() records everything committed so far.
    AcqRecorder other = std::move(recorder);
    QVERIFY(!recorder.isRecording()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): a moved-from recorder is empty
    QVERIFY(other.stop().ok());
    QVERIFY(other.stop().ok());
    QVERIFY(!other.isRecording());
    QCOMPARE(other.stats().samples, std::uint64_t(1500));
    QCOMPARE(other.stats().buffers, std::uint64_t(2)); // 1 s default buffer
    auto second = AcqRecorder::start(writer->stream(), m_dir.filePath(QStringLiteral("second_raw.fif")).toStdString());
    QVERIFY(second);
    other = std::move(*second);
    QVERIFY(other.isRecording());
    other = AcqRecorder();
    FiffRawData raw(path.toStdString());
    QCOMPARE(raw.last_samp, 1499);
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    if (argc == 3 && std::strcmp(argv[1], "--record-child") == 0) {
        return recordChild(argv[2]);
    }
    QCoreApplication app(argc, argv);
    TestAcqRecorder test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_acq_recorder.moc"
