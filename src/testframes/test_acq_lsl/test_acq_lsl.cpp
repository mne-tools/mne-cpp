//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_lsl.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the lsl:// source on both paths, against outlets of liblsl, pylsl and the in-tree LSL.
 *
 * The liblsl cases need a liblsl: MNE_CPP_LIBLSL, or pylsl installed for the python3 on the PATH; they skip
 * otherwise. Every outlet gets a unique name, so concurrent runs and other LSL streams on the network do not
 * interfere.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_device.h>
#include <acq/acq_device_registry.h>
#include <acq/acq_reader.h>
#include <acq/acq_stream.h>
#include <acq/detail/acq_shared_library.h>
#include <acq/lsl/acq_lsl.h>

#include <lsl/lsl_stream_info.h>
#include <lsl/lsl_stream_outlet.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QProcess>
#include <QStandardPaths>
#include <QUuid>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;
using namespace std::chrono_literals;

namespace
{

constexpr int kChannels = 3;
constexpr double kSfreq = 500.0;

/** liblsl outlet functions the test needs to produce streams. */
struct Outlets
{
    AcqSharedLibrary library;
    void* (*createInfo)(const char*, const char*, int, double, int, const char*) = nullptr;
    void* (*desc)(void*) = nullptr;
    void* (*appendChild)(void*, const char*) = nullptr;
    void* (*appendChildValue)(void*, const char*, const char*) = nullptr;
    void* (*createOutlet)(void*, int, int) = nullptr;
    void (*destroyOutlet)(void*) = nullptr;
    void (*destroyInfo)(void*) = nullptr;
    int (*waitForConsumers)(void*, double) = nullptr;
    double (*localClock)() = nullptr;
    int (*pushF)(void*, const float*, unsigned long, const double*, int) = nullptr;
    int (*pushD)(void*, const double*, unsigned long, const double*, int) = nullptr;
    int (*pushI)(void*, const std::int32_t*, unsigned long, const double*, int) = nullptr;
    int (*pushS)(void*, const std::int16_t*, unsigned long, const double*, int) = nullptr;

    static std::unique_ptr<Outlets> load()
    {
        std::vector<std::string> candidates;
        candidates.push_back(acqEnvironment("MNE_CPP_LIBLSL"));
        const std::string pylsl = acqPythonPackageDir("pylsl", "lib");
        if (!pylsl.empty()) {
            candidates.push_back(pylsl + "/" + acqSharedLibraryName("lsl"));
        }
        auto library = AcqSharedLibrary::load(candidates);
        if (!library) {
            return nullptr;
        }
        auto o = std::make_unique<Outlets>();
        o->library = std::move(*library);
        const auto bind = [&o](auto& function, const char* symbol) {
            function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(o->library.symbol(symbol));
        };
        bind(o->createInfo, "lsl_create_streaminfo");
        bind(o->desc, "lsl_get_desc");
        bind(o->appendChild, "lsl_append_child");
        bind(o->appendChildValue, "lsl_append_child_value");
        bind(o->createOutlet, "lsl_create_outlet");
        bind(o->destroyOutlet, "lsl_destroy_outlet");
        bind(o->destroyInfo, "lsl_destroy_streaminfo");
        bind(o->waitForConsumers, "lsl_wait_for_consumers");
        bind(o->localClock, "lsl_local_clock");
        bind(o->pushF, "lsl_push_chunk_ftnp");
        bind(o->pushD, "lsl_push_chunk_dtnp");
        bind(o->pushI, "lsl_push_chunk_itnp");
        bind(o->pushS, "lsl_push_chunk_stnp");
        return o;
    }
};

/** Value of channel @p c at sample @p s, exact in every LSL format. */
double value(std::size_t s, std::size_t c)
{
    return static_cast<double>(static_cast<int>((s * 7 + c * 1000) % 3000) - 1500);
}

std::string uniqueName(const char* prefix)
{
    return prefix + QUuid::createUuid().toString(QUuid::Id128).left(12).toStdString();
}

/** Everything a gating reader delivered as doubles, frame-major, until @p frames arrived or 10 s passed. */
std::vector<double> drain(AcqReader& reader, std::size_t frames)
{
    std::vector<double> out;
    const auto info = reader.stream().info();
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (out.size() < frames * info->channels.size() && std::chrono::steady_clock::now() < deadline) {
        auto view = reader.acquire(1, 200ms);
        if (!view) {
            continue;
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i) {
                for (std::size_t c = 0; c < info->channels.size(); ++c) {
                    const std::byte* at = part.at(c, i);
                    double v = 0.0;
                    switch (info->format) {
                        case AcqSampleFormat::Int16: {
                            std::int16_t x = 0;
                            std::memcpy(&x, at, sizeof(x));
                            v = x;
                            break;
                        }
                        case AcqSampleFormat::Int32: {
                            std::int32_t x = 0;
                            std::memcpy(&x, at, sizeof(x));
                            v = x;
                            break;
                        }
                        case AcqSampleFormat::Float32: {
                            float x = 0.0F;
                            std::memcpy(&x, at, sizeof(x));
                            v = x;
                            break;
                        }
                        default:
                            std::memcpy(&v, at, sizeof(v));
                            break;
                    }
                    out.push_back(v);
                }
            }
        }
        static_cast<void>(reader.release(*view));
    }
    return out;
}

/**
 * @return steady_clock minus @p localClock, in nanoseconds, from readings taken as one instant: the steady reading
 *         between two clock readings against their midpoint, the narrowest of a few tries, so that a preempted test
 *         thread does not shift the expected value.
 */
template<typename LocalClock>
double steadyMinusLocalNs(LocalClock localClock)
{
    double best = 0;
    double width = std::numeric_limits<double>::infinity();
    for (int attempt = 0; attempt < 5; ++attempt) {
        const double before = localClock();
        const auto steady = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::steady_clock::now().time_since_epoch())
                                .count();
        const double after = localClock();
        if (after - before < width) {
            width = after - before;
            best = static_cast<double>(steady) - 0.5 * (before + after) * 1e9;
        }
    }
    return best;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestAcqLsl
 */
class TestAcqLsl : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readsLiblslOutlets_data();
    void readsLiblslOutlets();
    void readsPylslOutlet();
    void readsInTreeOutlet();
    void readsInTreeOutlet_data();
    void pylslReadsInTreeOutlet();
    void simulatesAStream_data();
    void simulatesAStream();
    void rejectsBadUris();

private:
    std::unique_ptr<Outlets> m_lsl;
};

//=============================================================================================================

void TestAcqLsl::initTestCase()
{
    QVERIFY(registerLslScheme().ok());
    m_lsl = Outlets::load();
    qInfo("LSL runtime: %s", lslRuntime().c_str());
}

//=============================================================================================================

void TestAcqLsl::readsLiblslOutlets_data()
{
    QTest::addColumn<int>("lslFormat");
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("library");
    for (const char* library : {"liblsl", "intree"}) {
        const bool intree = std::string(library) == "intree";
        const auto row = [&](const char* name) -> QTestData& {
            return QTest::addRow("%s %s", library, name);
        };
        row("float32") << 1 << int(AcqSampleFormat::Float32) << QString(library);
        row("double64") << 2 << int(intree ? AcqSampleFormat::Float32 : AcqSampleFormat::Float64) << QString(library);
        row("int32") << 4 << int(intree ? AcqSampleFormat::Float32 : AcqSampleFormat::Int32) << QString(library);
        row("int16") << 5 << int(intree ? AcqSampleFormat::Float32 : AcqSampleFormat::Int16) << QString(library);
    }
}

void TestAcqLsl::readsLiblslOutlets()
{
    if (!m_lsl) {
        QSKIP("liblsl not found (set MNE_CPP_LIBLSL or pip install pylsl)");
    }
    QFETCH(int, lslFormat);
    QFETCH(int, format);
    QFETCH(QString, library);
    const bool intree = library == QStringLiteral("intree");
    const std::string name = uniqueName("acqlsl");
    void* info = m_lsl->createInfo(name.c_str(), "EEG", kChannels, kSfreq, lslFormat, name.c_str());
    void* channels = m_lsl->appendChild(m_lsl->desc(info), "channels");
    for (const char* label : {"Fz", "Cz", "Pz"}) {
        void* channel = m_lsl->appendChild(channels, "channel");
        m_lsl->appendChildValue(channel, "label", label);
        m_lsl->appendChildValue(channel, "unit", "microvolts");
        m_lsl->appendChildValue(channel, "type", "EEG");
    }
    void* outlet = m_lsl->createOutlet(info, 0, 360);
    m_lsl->destroyInfo(info);

    auto device = AcqDevice::open("lsl://" + name + "?timeout=10&library=" + library.toStdString());
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backendVersion.rfind(intree ? "in-tree" : "liblsl", 0), std::size_t(0));
    const auto streamInfo = device->stream().info();
    QCOMPARE(int(streamInfo->format), format);
    QCOMPARE(streamInfo->sfreq, kSfreq);
    QCOMPARE(streamInfo->channels.size(), std::size_t(kChannels));
    QCOMPARE(streamInfo->channels[1].kind, AcqChannelKind::Eeg);
    if (!intree) {
        QCOMPARE(streamInfo->channels[1].name, std::string("Cz"));
        QCOMPARE(streamInfo->channels[1].unit, AcqUnit::Volt);
        QCOMPARE(streamInfo->channels[1].unitExponent, -6);
    }

    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    constexpr std::size_t kFrames = 2000;
    const double t0 = m_lsl->localClock() + 0.5;
    std::vector<double> stamps(kFrames);
    std::vector<double> d(kFrames * kChannels);
    for (std::size_t s = 0; s < kFrames; ++s) {
        stamps[s] = t0 + static_cast<double>(s) / kSfreq;
        for (std::size_t c = 0; c < kChannels; ++c) {
            d[s * kChannels + c] = value(s, c);
        }
    }
    const auto elements = static_cast<unsigned long>(d.size());
    if (lslFormat == 1) {
        const std::vector<float> f(d.begin(), d.end());
        m_lsl->pushF(outlet, f.data(), elements, stamps.data(), 1);
    } else if (lslFormat == 2) {
        m_lsl->pushD(outlet, d.data(), elements, stamps.data(), 1);
    } else if (lslFormat == 4) {
        const std::vector<std::int32_t> i(d.begin(), d.end());
        m_lsl->pushI(outlet, i.data(), elements, stamps.data(), 1);
    } else {
        const std::vector<std::int16_t> i(d.begin(), d.end());
        m_lsl->pushS(outlet, i.data(), elements, stamps.data(), 1);
    }
    const std::vector<double> got = drain(*reader, kFrames);
    QCOMPARE(got.size(), d.size());
    QVERIFY(got == d);
    QCOMPARE(reader->stats().lost, std::uint64_t(0));

    // The clock maps stream samples to the LSL timestamps of the outlet (same host: correction ~0).
    const AcqClockModel clock = reader->stream().clock();
    QCOMPARE(clock.source, AcqClockSource::Lsl);
    const double expected = steadyMinusLocalNs([&] { return m_lsl->localClock(); }) + stamps[clock.anchorSample] * 1e9;
    QVERIFY2(std::abs(static_cast<double>(clock.anchorHostNs) - expected) < 5e6,
             qPrintable(QString("clock off by %1 ms").arg((static_cast<double>(clock.anchorHostNs) - expected) / 1e6)));

    device->close();
    m_lsl->destroyOutlet(outlet);
}

//=============================================================================================================

void TestAcqLsl::readsPylslOutlet()
{
    QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (python.isEmpty()) {
        python = QStandardPaths::findExecutable(QStringLiteral("python"));
    }
    if (python.isEmpty() || !m_lsl) {
        QSKIP("pylsl with its liblsl not installed for the Python on the PATH");
    }
    const std::string name = uniqueName("pylsl");
    // The outlet waits for our inlet, then pushes 1000 float32 samples of value(s, c) and lingers until read.
    const QString script = QStringLiteral(
                               "import pylsl,time\n"
                               "info=pylsl.StreamInfo('%1','EEG',3,500.0,'float32','%1')\n"
                               "ch=info.desc().append_child('channels')\n"
                               "[ch.append_child('channel').append_child_value('label',n) for n in ('Fz','Cz','Pz')]\n"
                               "out=pylsl.StreamOutlet(info)\n"
                               "assert out.wait_for_consumers(20)\n"
                               "time.sleep(0.5)\n"
                               "out.push_chunk([[float((s*7+c*1000)%3000-1500) for c in range(3)] for s in range(1000)])\n"
                               "time.sleep(5)\n")
                               .arg(QString::fromStdString(name));
    QProcess outlet;
    outlet.setProcessChannelMode(QProcess::ForwardedChannels);
    outlet.start(python, {QStringLiteral("-c"), script});
    QVERIFY(outlet.waitForStarted(10000));

    auto device = AcqDevice::open("lsl://" + name + "?timeout=20");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->stream().info()->channels[2].name, std::string("Pz"));
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    const std::vector<double> got = drain(*reader, 1000);
    QCOMPARE(got.size(), std::size_t(1000 * kChannels));
    for (std::size_t i = 0; i < got.size(); ++i) {
        QCOMPARE(got[i], value(i / kChannels, i % kChannels));
    }
    device->close();
    outlet.kill();
    outlet.waitForFinished(5000);
}

//=============================================================================================================

void TestAcqLsl::readsInTreeOutlet_data()
{
    QTest::addColumn<int>("lslFormat");
    QTest::addColumn<QString>("library");
    for (const char* library : {"intree", "liblsl"}) {
        for (const auto& [format, label] : {std::pair{1, "float32"}, std::pair{2, "double64"}, std::pair{4, "int32"},
                                            std::pair{5, "int16"}, std::pair{6, "int8"}, std::pair{7, "int64"}}) {
            QTest::addRow("%s %s", library, label) << format << QString(library);
        }
    }
}

void TestAcqLsl::readsInTreeOutlet()
{
    QFETCH(int, lslFormat);
    QFETCH(QString, library);
    if (library == QStringLiteral("liblsl") && !m_lsl) {
        QSKIP("liblsl not found");
    }
    const std::string name = uniqueName("intree");
    mne::stream_outlet outlet(
        mne::stream_info(name, "EEG", kChannels, kSfreq, static_cast<mne::ChannelFormat>(lslFormat), name));
    auto device = AcqDevice::open("lsl://" + name + "?timeout=5&library=" + library.toStdString());
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    QTRY_VERIFY_WITH_TIMEOUT(outlet.have_consumers(), 5000);
    // Values within +-1500 are exact in every format, int8 excepted: the outlet wraps them to its range.
    const auto expected = [lslFormat](std::size_t s, std::size_t c) {
        const double v = value(s, c);
        return lslFormat == 6 ? static_cast<double>(static_cast<std::int8_t>(static_cast<int>(v))) : v;
    };
    std::vector<std::vector<float>> chunk(500, std::vector<float>(kChannels));
    for (std::size_t s = 0; s < chunk.size(); ++s) {
        for (std::size_t c = 0; c < kChannels; ++c) {
            chunk[s][c] = static_cast<float>(value(s, c));
        }
    }
    const double pushed = mne::local_clock();
    outlet.push_chunk(chunk);
    const std::vector<double> got = drain(*reader, chunk.size());
    QCOMPARE(got.size(), chunk.size() * kChannels);
    for (std::size_t i = 0; i < got.size(); ++i) {
        QCOMPARE(got[i], expected(i / kChannels, i % kChannels));
    }
    // The outlet stamps the last sample of a chunk with the push time and the others back by 1 / rate.
    const AcqClockModel clock = reader->stream().clock();
    QCOMPARE(clock.source, AcqClockSource::Lsl);
    const double stamp = pushed - static_cast<double>(chunk.size() - 1 - clock.anchorSample) / kSfreq;
    const double want = steadyMinusLocalNs([] { return mne::local_clock(); }) + stamp * 1e9;
    QVERIFY2(std::abs(static_cast<double>(clock.anchorHostNs) - want) < 5e6,
             qPrintable(QString("clock off by %1 ms").arg((static_cast<double>(clock.anchorHostNs) - want) / 1e6)));
    device->close();
}

//=============================================================================================================

void TestAcqLsl::pylslReadsInTreeOutlet()
{
    QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (python.isEmpty()) {
        python = QStandardPaths::findExecutable(QStringLiteral("python"));
    }
    if (python.isEmpty() || !m_lsl) {
        QSKIP("pylsl with its liblsl not installed for the Python on the PATH");
    }
    const std::string name = uniqueName("topylsl");
    mne::stream_outlet outlet(mne::stream_info(name, "EEG", kChannels, kSfreq, mne::ChannelFormat::Int16, name));
    // pylsl resolves the outlet by name, checks its description, reads 300 samples and prints them.
    const QString script = QStringLiteral(
                               "import pylsl,sys\n"
                               "s=pylsl.resolve_byprop('name','%1',1,10)\n"
                               "assert s and s[0].channel_format()==pylsl.cf_int16 and s[0].nominal_srate()==500.0\n"
                               "i=pylsl.StreamInlet(s[0])\n"
                               "i.open_stream(10)\n"
                               "assert abs(i.time_correction(5))<0.01\n"
                               "out=[]\n"
                               "while len(out)<300:\n"
                               "    d,t=i.pull_chunk(timeout=5)\n"
                               "    assert d, 'timeout'\n"
                               "    out+=d\n"
                               "print(' '.join(str(v) for row in out[:300] for v in row))\n")
                               .arg(QString::fromStdString(name));
    QProcess inlet;
    inlet.start(python, {QStringLiteral("-c"), script});
    QVERIFY(inlet.waitForStarted(10000));
    QTRY_VERIFY_WITH_TIMEOUT(outlet.have_consumers(), 20000);
    std::vector<std::vector<float>> chunk(300, std::vector<float>(kChannels));
    for (std::size_t s = 0; s < chunk.size(); ++s) {
        for (std::size_t c = 0; c < kChannels; ++c) {
            chunk[s][c] = static_cast<float>(value(s, c));
        }
    }
    outlet.push_chunk(chunk);
    QVERIFY(inlet.waitForFinished(30000));
    const QByteArray error = inlet.readAllStandardError();
    QVERIFY2(inlet.exitCode() == 0, error.constData());
    const QList<QByteArray> values = inlet.readAllStandardOutput().trimmed().split(' ');
    QCOMPARE(values.size(), qsizetype(chunk.size() * kChannels));
    for (qsizetype i = 0; i < values.size(); ++i) {
        QCOMPARE(values[i].toDouble(), value(static_cast<std::size_t>(i) / kChannels, static_cast<std::size_t>(i) % kChannels));
    }
}

//=============================================================================================================

void TestAcqLsl::simulatesAStream_data()
{
    QTest::addColumn<QString>("library");
    QTest::newRow("intree") << QStringLiteral("intree");
    QTest::newRow("liblsl") << QStringLiteral("liblsl");
}

void TestAcqLsl::simulatesAStream()
{
    QFETCH(QString, library);
    if (library == QStringLiteral("liblsl") && !m_lsl) {
        QSKIP("liblsl not found");
    }
    // simulate=1 publishes a synthetic EEG session as an LSL outlet and reads it back, without a target.
    auto device = AcqDevice::open("lsl://?simulate=1&channels=4&sfreq=250&library=" + library.toStdString());
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    const auto info = reader->stream().info();
    QCOMPARE(info->channels.size(), std::size_t{4});
    QCOMPARE(info->sfreq, 250.0);
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Volt);
    QVERIFY(device->start().ok());
    const std::vector<double> got = drain(*reader, 125);
    QVERIFY(got.size() >= std::size_t{125 * 4});
    double peak = 0.0;
    for (const double v : got) {
        peak = std::max(peak, std::abs(v));
    }
    // Scalp EEG in volts: tens of microvolts
    QVERIFY2(peak > 1e-6 && peak < 1e-3, qPrintable(QString::number(peak)));
    device->close();
}

//=============================================================================================================

void TestAcqLsl::rejectsBadUris()
{
    const auto code = [](const std::string& uri) {
        auto device = AcqDevice::open(uri);
        return device ? AcqErrorCode::None : device.error().code();
    };
    QCOMPARE(code("lsl://"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://x?library=other"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://x?timeout=-1"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://x?colour=red"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://?simulate=1&channels=0"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://?simulate=1&sfreq=0"), AcqErrorCode::InvalidParameter);
    QCOMPARE(code("lsl://" + uniqueName("absent") + "?timeout=0.2&library=intree"), AcqErrorCode::DeviceNotFound);
    if (m_lsl) {
        QCOMPARE(code("lsl://" + uniqueName("absent") + "?timeout=0.2"), AcqErrorCode::DeviceNotFound);
        const std::string name = uniqueName("markers");
        void* info = m_lsl->createInfo(name.c_str(), "Markers", 1, 0.0, 3, name.c_str());
        void* outlet = m_lsl->createOutlet(info, 0, 360);
        m_lsl->destroyInfo(info);
        QCOMPARE(code("lsl://" + name + "?timeout=5"), AcqErrorCode::UnsupportedFormat);
        m_lsl->destroyOutlet(outlet);
    }
    bool listed = false;
    for (const AcqSchemeInfo& scheme : AcqDeviceRegistry::instance().schemes()) {
        listed = listed || scheme.scheme == "lsl";
    }
    QVERIFY(listed);
}

QTEST_GUILESS_MAIN(TestAcqLsl)
#include "test_acq_lsl.moc"
