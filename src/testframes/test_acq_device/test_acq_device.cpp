//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_device.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests for device URIs, the scheme registry, the device life cycle and the synthetic:// source.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_device.h>
#include <acq/acq_device_registry.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <set>
#include <string>
#include <thread>
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

AcqStreamInfo channelsInfo(std::string name, AcqDirection direction, std::size_t channels)
{
    AcqStreamInfo info;
    info.name = std::move(name);
    info.direction = direction;
    info.format = AcqSampleFormat::Float32;
    info.sfreq = 1000.0;
    info.channels.resize(channels);
    for (std::size_t c = 0; c < channels; ++c) {
        info.channels[c].name = "ch" + std::to_string(c);
    }
    return info;
}

/** What the device asked of a FakeBackend; outlives the backend, which close() destroys. */
struct Calls
{
    AcqError startResult;
    int starts = 0;
    int stops = 0;
    int closes = 0;
};

/** Backend whose behaviour the tests control. */
class FakeBackend : public AcqBackend
{
public:
    explicit FakeBackend(Calls& record)
    : calls(record)
    {
        input = addInputStream(channelsInfo("in", AcqDirection::Input, 2)).value();
        deviceSide = addOutputStream(channelsInfo("out", AcqDirection::Output, 1), {}, AcqWait::Block).value();
    }

    AcqDeviceDescription description() const override
    {
        return {"fake", "", "test device", "fake://"};
    }

    AcqError start() override
    {
        ++calls.starts;
        return calls.startResult;
    }

    void stop() override
    {
        ++calls.stops;
    }

    void close() override
    {
        ++calls.closes;
    }

    AcqExpected<AcqWriter*> addAfterOpen()
    {
        return addInputStream(channelsInfo("late", AcqDirection::Input, 1));
    }

    AcqExpected<AcqReader*> addOutputAfterOpen()
    {
        return addOutputStream(channelsInfo("late", AcqDirection::Output, 1));
    }

    /** Error codes of stream creation while still opening: wrong direction and rejected options, both ways. */
    std::vector<AcqErrorCode> creationErrors()
    {
        AcqStreamOptions noReaders;
        noReaders.maxReaders = 0;
        return {addInputStream(channelsInfo("x", AcqDirection::Output, 1)).error().code(),
                addOutputStream(channelsInfo("x", AcqDirection::Input, 1)).error().code(),
                addInputStream(channelsInfo("x", AcqDirection::Input, 1), noReaders).error().code(),
                addOutputStream(channelsInfo("x", AcqDirection::Output, 1), noReaders).error().code()};
    }

    void fail(AcqError error)
    {
        reportError(std::move(error));
    }

    Calls& calls;
    AcqWriter* input = nullptr;
    AcqReader* deviceSide = nullptr;
};

/** Everything a Lossless reader delivered until EndOfStream: interleaved float32 frames and events. */
struct Drained
{
    std::vector<float> samples;
    std::vector<AcqEvent> events;
    std::uint64_t lost = 0;
    std::uint64_t writerWaits = 0; /**< Times the producer waited for this reader's stream. */
};

Drained drain(AcqReader& reader, std::size_t maxView = 0)
{
    Drained out;
    while (true) {
        auto view = reader.acquire(1, 5s, maxView);
        if (!view) {
            if (view.error() != AcqErrorCode::EndOfStream) {
                qWarning("acquire failed: %s", view.error().text().c_str());
            }
            break;
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i) {
                for (std::size_t c = 0; c < reader.channelCount(); ++c) {
                    float v = 0.0F;
                    std::memcpy(&v, part.at(c, i), sizeof(v));
                    out.samples.push_back(v);
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            out.events.push_back(view->events()[e]);
        }
        static_cast<void>(reader.release(*view));
    }
    out.lost = reader.stats().lost;
    out.writerWaits = reader.stream().stats().writerWaits;
    return out;
}

/** Records every stream of @p uri until end of stream, one gating reader thread per stream (no loss). */
std::vector<Drained> recordAll(const std::string& uri)
{
    auto device = AcqDevice::open(uri);
    if (!device) {
        qWarning("open(%s): %s", uri.c_str(), device.error().text().c_str());
        return {};
    }
    AcqReaderOptions gating;
    gating.gating = true;
    std::vector<AcqReader> readers;
    for (const AcqStream& stream : device->streams()) {
        auto reader = stream.reader(AcqReader::ReadMode::Lossless, gating);
        if (!reader) {
            return {};
        }
        readers.push_back(std::move(*reader));
    }
    std::vector<Drained> out(readers.size());
    std::vector<std::thread> threads;
    threads.reserve(readers.size());
    for (std::size_t i = 0; i < readers.size(); ++i) {
        threads.emplace_back([&out, &readers, i] { out[i] = drain(readers[i], 512); });
    }
    if (!device->start().ok()) {
        device->close();
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return out;
}

Drained record(const std::string& uri)
{
    auto device = AcqDevice::open(uri);
    if (!device) {
        qWarning("open(%s): %s", uri.c_str(), device.error().text().c_str());
        return {};
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader || !device->start().ok()) {
        return {};
    }
    return drain(*reader);
}

/** Trial-averaged value of channel ch over [from, to) seconds after every event of code. */
double windowMean(const Drained& d, std::size_t channels, std::size_t ch, double sfreq, std::int32_t code, double from,
                  double to)
{
    double sum = 0.0;
    std::size_t n = 0;
    const auto frames = static_cast<std::int64_t>(d.samples.size() / channels);
    for (const AcqEvent& e : d.events) {
        const auto a = static_cast<std::int64_t>(e.sample) + std::llround(from * sfreq);
        const auto b = static_cast<std::int64_t>(e.sample) + std::llround(to * sfreq);
        if (e.code != code || a < 0 || b > frames) {
            continue;
        }
        for (std::int64_t s = a; s < b; ++s) {
            sum += d.samples[static_cast<std::size_t>(s) * channels + ch];
            ++n;
        }
    }
    return n == 0 ? 0.0 : sum / static_cast<double>(n);
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestAcqDevice
 *
 * @brief Device URIs, scheme registry, device life cycle and the synthetic:// source.
 */
class TestAcqDevice : public QObject
{
    Q_OBJECT

private slots:
    void testRegistryListsCompiledSchemes();
    void testUriParse();
    void testUriInvalid_data();
    void testUriInvalid();
    void testUriRoundTrip();
    void testRegistryErrors();
    void testRegistryCustomScheme();
    void testDiscover();
    void testBackendStreamCreationErrors();
    void testDeviceLifeCycle();
    void testDeviceStartFailure();
    void testDeviceReportedError();
    void testDeviceOutputStream();
    void testDeviceStreamsLookup();
    void testDeviceMove();
    void testSyntheticDefaults();
    void testSyntheticDeterministic();
    void testSyntheticDurationAndEvents();
    void testSyntheticNoiseAndRhythm();
    void testSyntheticOddballResponses();
    void testSyntheticMeg();
    void testSyntheticRealtimePacing();
    void testSyntheticStopRestart();
    void testSyntheticInvalidParameters_data();
    void testSyntheticInvalidParameters();
    void testSyntheticMultiRateStreams();
    void testSyntheticMultiRateResponses();
    void testRealtimeRecordingIsComplete();
    void testBackpressureReplayIsLossless();
    void testImpedances();
};

//=============================================================================================================

void TestAcqDevice::testRegistryListsCompiledSchemes()
{
    const auto schemes = AcqDeviceRegistry::instance().schemes();
    QCOMPARE(schemes.size(), std::size_t(1));
    QCOMPARE(schemes[0].scheme, std::string("synthetic"));
    QVERIFY(!schemes[0].description.empty());
    QCOMPARE(schemes[0].exampleUri, std::string("synthetic://"));
    QVERIFY(schemes[0].parameters.size() >= 10);
}

//=============================================================================================================

void TestAcqDevice::testUriParse()
{
    auto uri = AcqUri::parse("FIFF:///data/raw%20one.fif?realtime=1&loop=0&start");
    QVERIFY(uri);
    QCOMPARE(uri->scheme, std::string("fiff"));
    QCOMPARE(uri->target, std::string("/data/raw one.fif"));
    QCOMPARE(uri->params.size(), std::size_t(3));
    QCOMPARE(uri->params[0].first, std::string("realtime"));
    QCOMPARE(uri->params[1].first, std::string("loop"));
    QCOMPARE(*uri->param("loop"), std::string_view("0"));
    QCOMPARE(*uri->param("start"), std::string_view(""));
    QVERIFY(!uri->param("missing").has_value());

    auto lsl = AcqUri::parse("lsl://EEG%3fstream?type=a%26b");
    QVERIFY(lsl);
    QCOMPARE(lsl->target, std::string("EEG?stream"));
    QCOMPARE(*lsl->param("type"), std::string_view("a&b"));

    // BrainFlow's streamer strings have an underscore in the scheme
    auto streamer = AcqUri::parse("streaming_board://225.1.1.1:6677");
    QVERIFY(streamer);
    QCOMPARE(streamer->scheme, std::string("streaming_board"));
    QCOMPARE(streamer->target, std::string("225.1.1.1:6677"));

    auto bare = AcqUri::parse("synthetic://");
    QVERIFY(bare);
    QVERIFY(bare->target.empty());
    QVERIFY(bare->params.empty());
}

//=============================================================================================================

void TestAcqDevice::testUriInvalid_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("empty") << "";
    QTest::newRow("no separator") << "synthetic";
    QTest::newRow("empty scheme") << "://x";
    QTest::newRow("digit first") << "1abc://";
    QTest::newRow("underscore first") << "_abc://";
    QTest::newRow("space in scheme") << "syn thetic://";
    QTest::newRow("empty key") << "a://x?=1";
    QTest::newRow("empty segment") << "a://x?k=1&&j=2";
    QTest::newRow("trailing ampersand") << "a://x?k=1&";
    QTest::newRow("duplicate key") << "a://x?k=1&k=2";
    QTest::newRow("short escape") << "a://x?k=%2";
    QTest::newRow("bad escape") << "a://x%zz";
    QTest::newRow("bad key escape") << "a://x?k%g=1";
}

void TestAcqDevice::testUriInvalid()
{
    QFETCH(QString, uri);
    auto parsed = AcqUri::parse(uri.toStdString());
    QVERIFY(!parsed);
    QCOMPARE(parsed.error().code(), AcqErrorCode::InvalidUri);
    QVERIFY(!parsed.error().context().empty());
}

//=============================================================================================================

void TestAcqDevice::testUriRoundTrip()
{
    AcqUri uri;
    uri.scheme = "fiff";
    uri.target = "/data/raw one.fif";
    uri.params = {{"realtime", "1"}, {"label", "a&b=c?"}};
    const std::string text = uri.toString();
    QCOMPARE(text, std::string("fiff:///data/raw%20one.fif?realtime=1&label=a%26b%3Dc%3F"));
    auto back = AcqUri::parse(text);
    QVERIFY(back);
    QCOMPARE(back->target, uri.target);
    QCOMPARE(back->params, uri.params);
    QCOMPARE(AcqUri::parse("C:/x").error().code(), AcqErrorCode::InvalidUri);
    QCOMPARE(AcqUri::parse("fiff://C:/data/raw.fif")->target, std::string("C:/data/raw.fif"));
}

//=============================================================================================================

void TestAcqDevice::testRegistryErrors()
{
    auto unknown = AcqDevice::open("nosuch://x");
    QCOMPARE(unknown.error().code(), AcqErrorCode::UnknownScheme);
    QVERIFY(unknown.error().context().find("nosuch") != std::string_view::npos);

    QCOMPARE(AcqDevice::open("not a uri").error().code(), AcqErrorCode::InvalidUri);

    auto bogus = AcqDevice::open("synthetic://?bogus=1");
    QCOMPARE(bogus.error().code(), AcqErrorCode::InvalidParameter);
    QVERIFY(bogus.error().context().find("bogus") != std::string_view::npos);

    AcqDeviceRegistry& registry = AcqDeviceRegistry::instance();
    auto factory = [](const AcqUri&) -> AcqExpected<std::unique_ptr<AcqBackend>> {
        return AcqErrorCode::DeviceNotFound;
    };
    QCOMPARE(registry.registerScheme({"synthetic", "again", "", {}}, factory).code(), AcqErrorCode::InvalidParameter);
    QCOMPARE(registry.registerScheme({"Bad Scheme", "", "", {}}, factory).code(), AcqErrorCode::InvalidParameter);
    QCOMPARE(registry.registerScheme({"ab cd", "", "", {}}, factory).code(), AcqErrorCode::InvalidParameter);
    QCOMPARE(registry.registerScheme({"_ab", "", "", {}}, factory).code(), AcqErrorCode::InvalidParameter);
    QVERIFY(registry.registerScheme({"under_score", "", "", {}}, factory).ok());
    QCOMPARE(registry.registerScheme({"noop", "", "", {}}, {}).code(), AcqErrorCode::InvalidParameter);

    QVERIFY(registry.registerScheme({"nullbackend", "", "", {}}, [](const AcqUri&) -> AcqExpected<std::unique_ptr<AcqBackend>> {
                        return std::unique_ptr<AcqBackend>();
                    })
                .ok());
    QCOMPARE(registry.open("nullbackend://").error().code(), AcqErrorCode::Internal);
    QCOMPARE(registry.open("nullbackend://x").error().code(), AcqErrorCode::Internal);
    registry.registerScheme({"failing", "", "", {}}, factory);
    QCOMPARE(registry.open("failing://").error().code(), AcqErrorCode::DeviceNotFound);
}

//=============================================================================================================

void TestAcqDevice::testRegistryCustomScheme()
{
    AcqDeviceRegistry& registry = AcqDeviceRegistry::instance();
    static Calls registeredCalls;
    std::string seenTarget;
    const AcqError added = registry.registerScheme(
        {"fake", "Test backend.", "fake://", {{"mode", "string", "a", "Mode."}}},
        [&seenTarget](const AcqUri& uri) -> AcqExpected<std::unique_ptr<AcqBackend>> {
            seenTarget = uri.target;
            return std::unique_ptr<AcqBackend>(new FakeBackend(registeredCalls));
        },
        [](std::chrono::milliseconds) { return std::vector<AcqDiscoveredDevice>{{"fake://box", "Fake box", "fake", "", true}}; });
    QVERIFY(added.ok());

    auto device = registry.open("fake://box?mode=b");
    QVERIFY(device);
    QCOMPARE(seenTarget, std::string("box"));
    QCOMPARE(device->description().backend, std::string("fake"));
    QCOMPARE(device->state(), AcqDeviceState::Opened);

    const auto names = registry.schemes();
    QVERIFY(std::is_sorted(names.begin(), names.end(),
                           [](const AcqSchemeInfo& a, const AcqSchemeInfo& b) { return a.scheme < b.scheme; }));

    const auto found = registry.discover(2s);
    QVERIFY(std::any_of(found.begin(), found.end(), [](const AcqDiscoveredDevice& d) { return d.uri == "fake://box"; }));
}

//=============================================================================================================

void TestAcqDevice::testDiscover()
{
    const auto found = AcqDeviceRegistry::instance().discover(2s);
    auto it = std::find_if(found.begin(), found.end(), [](const AcqDiscoveredDevice& d) { return d.scheme == "synthetic"; });
    QVERIFY(it != found.end());
    QVERIFY(it->available);
    QVERIFY(!it->detail.empty());
    QVERIFY(AcqDevice::open(it->uri));
}

//=============================================================================================================

void TestAcqDevice::testBackendStreamCreationErrors()
{
    Calls calls;
    FakeBackend fake(calls);
    const std::vector<AcqErrorCode> expected = {AcqErrorCode::WrongDirection, AcqErrorCode::WrongDirection,
                                                AcqErrorCode::InvalidParameter, AcqErrorCode::InvalidParameter};
    QCOMPARE(fake.creationErrors(), expected);
    QCOMPARE(fake.streams().size(), std::size_t(2));
}

//=============================================================================================================

void TestAcqDevice::testDeviceLifeCycle()
{
    Calls calls;
    auto* fake = new FakeBackend(calls);
    AcqDevice device{std::unique_ptr<AcqBackend>(fake)};
    QCOMPARE(device.state(), AcqDeviceState::Opened);
    QCOMPARE(fake->addAfterOpen().error().code(), AcqErrorCode::InvalidState);
    QCOMPARE(fake->addOutputAfterOpen().error().code(), AcqErrorCode::InvalidState);

    QVERIFY(device.stop().ok());
    QCOMPARE(calls.stops, 0);

    QVERIFY(device.start().ok());
    QCOMPARE(device.state(), AcqDeviceState::Running);
    QCOMPARE(device.start().code(), AcqErrorCode::InvalidState);

    auto reader = device.stream(0).reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    auto view = fake->input->beginWrite(4);
    QVERIFY(view);
    fake->input->commit(4);

    QVERIFY(device.stop().ok());
    QCOMPARE(device.state(), AcqDeviceState::Stopped);
    QCOMPARE(calls.stops, 1);
    QVERIFY(device.start().ok());
    QCOMPARE(calls.starts, 2);

    const AcqDeviceStats stats = device.stats();
    QCOMPARE(stats.state, AcqDeviceState::Running);
    QCOMPARE(stats.streams.size(), std::size_t(2));
    QCOMPARE(stats.streams[0].committed, std::uint64_t(4));

    device.close();
    QCOMPARE(device.state(), AcqDeviceState::Closed);
    QCOMPARE(calls.stops, 2);
    QVERIFY(device.streams().empty());
    QCOMPARE(device.start().code(), AcqErrorCode::InvalidState);
    device.close();

    // Closing finished the streams: the reader drains the committed samples, then sees the end.
    auto data = reader->acquire(1, 1s);
    QVERIFY(data);
    QCOMPARE(data->sampleCount(), std::size_t(4));
    QVERIFY(reader->release(*data).ok());
    QCOMPARE(reader->acquire(1, 1s).error().code(), AcqErrorCode::EndOfStream);
}

//=============================================================================================================

void TestAcqDevice::testDeviceStartFailure()
{
    Calls calls;
    auto* fake = new FakeBackend(calls);
    calls.startResult = AcqError(AcqErrorCode::DeviceNotFound, "serial 42");
    AcqDevice device{std::unique_ptr<AcqBackend>(fake)};
    QCOMPARE(device.start().code(), AcqErrorCode::DeviceNotFound);
    QCOMPARE(device.state(), AcqDeviceState::Opened);
    QCOMPARE(device.lastError().code(), AcqErrorCode::DeviceNotFound);
    QCOMPARE(device.lastError().context(), std::string_view("serial 42"));
    QVERIFY(device.stop().ok());
    QCOMPARE(calls.stops, 0);
}

//=============================================================================================================

void TestAcqDevice::testDeviceReportedError()
{
    Calls calls;
    auto* fake = new FakeBackend(calls);
    AcqDevice device{std::unique_ptr<AcqBackend>(fake)};
    QVERIFY(device.start().ok());
    std::thread worker([fake] {
        fake->fail(AcqError(AcqErrorCode::DeviceIo, "cable"));
        fake->fail(AcqError(AcqErrorCode::Internal));
    });
    worker.join();
    QCOMPARE(device.state(), AcqDeviceState::Failed);
    QCOMPARE(device.lastError().code(), AcqErrorCode::DeviceIo);
    QCOMPARE(device.lastError().context(), std::string_view("cable"));
    QCOMPARE(device.start().code(), AcqErrorCode::InvalidState);
    QCOMPARE(device.stop().code(), AcqErrorCode::DeviceIo);
    QCOMPARE(calls.stops, 1);
    device.close();
    QCOMPARE(calls.closes, 1);
    QCOMPARE(device.state(), AcqDeviceState::Closed);
}

//=============================================================================================================

void TestAcqDevice::testDeviceOutputStream()
{
    Calls calls;
    auto* fake = new FakeBackend(calls);
    AcqDevice device{std::unique_ptr<AcqBackend>(fake)};
    AcqStream out = device.findStream("out");
    QCOMPARE(out.direction(), AcqDirection::Output);
    auto writer = out.writer();
    QVERIFY(writer);
    QCOMPARE(out.writer().error().code(), AcqErrorCode::WriterInUse);

    auto view = writer->beginWrite(3, 1s);
    QVERIFY(view);
    const float values[3] = {1.0F, 2.0F, 3.0F};
    for (std::size_t i = 0; i < 3; ++i) {
        std::memcpy(view->part(0).at(0, i), &values[i], sizeof(float));
    }
    writer->commit(3);

    auto played = fake->deviceSide->acquire(3, 1s);
    QVERIFY(played);
    QCOMPARE(played->sampleCount(), std::size_t(3));
    std::vector<float> got;
    for (std::size_t p = 0; p < played->partCount(); ++p) {
        for (std::size_t i = 0; i < played->part(p).samples; ++i) {
            float v = 0.0F;
            std::memcpy(&v, played->part(p).at(0, i), sizeof(v));
            got.push_back(v);
        }
    }
    QCOMPARE(got, (std::vector<float>{1.0F, 2.0F, 3.0F}));
    QVERIFY(fake->deviceSide->release(*played).ok());
}

//=============================================================================================================

void TestAcqDevice::testDeviceStreamsLookup()
{
    Calls calls;
    AcqDevice device{std::unique_ptr<AcqBackend>(new FakeBackend(calls))};
    QCOMPARE(device.streams().size(), std::size_t(2));
    QCOMPARE(device.stream(0).info()->name, std::string("in"));
    QCOMPARE(device.stream(1).info()->name, std::string("out"));
    QVERIFY(!device.stream(2).isValid());
    QVERIFY(device.findStream("in").isValid());
    QVERIFY(!device.findStream("none").isValid());
    QVERIFY(device.measureImpedances().error() == AcqErrorCode::NotSupported); // the AcqBackend default

    AcqDevice empty;
    QCOMPARE(empty.state(), AcqDeviceState::Closed);
    QVERIFY(empty.streams().empty());
    QCOMPARE(empty.start().code(), AcqErrorCode::InvalidState);
    QVERIFY(empty.stop().ok());
    QVERIFY(empty.lastError().ok());
    empty.close();
    QVERIFY(empty.description().backend.empty());
    QVERIFY(empty.stats().streams.empty());
}

//=============================================================================================================

void TestAcqDevice::testDeviceMove()
{
    Calls calls;
    auto* fake = new FakeBackend(calls);
    AcqDevice a{std::unique_ptr<AcqBackend>(fake)};
    QVERIFY(a.start().ok());
    AcqDevice b = std::move(a);
    QCOMPARE(b.state(), AcqDeviceState::Running);
    AcqDevice c{std::unique_ptr<AcqBackend>(new FakeBackend(calls))};
    c = std::move(b);
    QCOMPARE(c.state(), AcqDeviceState::Running);
    QCOMPARE(calls.stops, 0);
    c.close();
    QCOMPARE(calls.stops, 1);
}

//=============================================================================================================

void TestAcqDevice::testSyntheticDefaults()
{
    auto device = AcqDevice::open("synthetic://");
    QVERIFY(device);
    const AcqDeviceDescription description = device->description();
    QCOMPARE(description.backend, std::string("synthetic"));
    QVERIFY(AcqUri::parse(description.uri));
    QCOMPARE(device->streams().size(), std::size_t(1));

    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->channelCount(), std::size_t(9));
    for (std::size_t c = 0; c < 8; ++c) {
        QCOMPARE(info->channels[c].kind, AcqChannelKind::Eeg);
        QCOMPARE(info->channels[c].unit, AcqUnit::Volt);
    }
    QCOMPARE(info->channels[8].kind, AcqChannelKind::Stim);
    QCOMPARE(info->channels[8].name, std::string("STI 014"));
    QCOMPARE(info->eventLabels.size(), std::size_t(3));
    QCOMPARE(info->eventLabels[1], std::string("standard"));
    QCOMPARE(info->eventLabels[2], std::string("target"));
    QVERIFY(device->start().ok());
    device->close();
}

//=============================================================================================================

void TestAcqDevice::testSyntheticDeterministic()
{
    const Drained a = record("synthetic://?realtime=0&duration=3&seed=5&block=7");
    const Drained b = record("synthetic://?realtime=0&duration=3&seed=5&block=100");
    const Drained c = record("synthetic://?realtime=0&duration=3&seed=6");
    QCOMPARE(a.samples.size(), std::size_t(3000 * 9));
    QVERIFY(a.samples == b.samples);
    QCOMPARE(a.events.size(), b.events.size());
    for (std::size_t i = 0; i < a.events.size(); ++i) {
        QCOMPARE(a.events[i].sample, b.events[i].sample);
        QCOMPARE(a.events[i].code, b.events[i].code);
    }
    QVERIFY(a.samples != c.samples);
}

//=============================================================================================================

void TestAcqDevice::testSyntheticDurationAndEvents()
{
    const Drained d = record("synthetic://?realtime=0&duration=20&sfreq=500&interval=0.5");
    const std::size_t channels = 9;
    QCOMPARE(d.samples.size(), std::size_t(10000 * channels));
    QCOMPARE(d.lost, std::uint64_t(0));
    // Trial k starts at (k + 1) x interval plus a jitter below 10 % of the interval.
    QCOMPARE(d.events.size(), std::size_t(39));
    std::uint64_t previous = 0;
    for (const AcqEvent& e : d.events) {
        QVERIFY(e.code == 1 || e.code == 2);
        QVERIFY(e.sample > previous);
        QCOMPARE(e.duration, std::uint32_t(3)); // 5 ms at 500 Hz, rounded up
        previous = e.sample;
        for (std::uint64_t s = e.sample; s < e.sample + e.duration; ++s) {
            QCOMPARE(d.samples[s * channels + 8], static_cast<float>(e.code));
        }
        QCOMPARE(d.samples[(e.sample - 1) * channels + 8], 0.0F);
    }
}

//=============================================================================================================

void TestAcqDevice::testSyntheticNoiseAndRhythm()
{
    // Noise only: 10 uV rms, zero mean, uncorrelated between channels.
    const Drained n = record("synthetic://?realtime=no&duration=20&alpha=0&interval=0&stim=0&eeg=2");
    const std::size_t frames = n.samples.size() / 2;
    QCOMPARE(frames, std::size_t(20000));
    double sum = 0.0, sq = 0.0, cross = 0.0;
    for (std::size_t s = 0; s < frames; ++s) {
        sum += n.samples[2 * s];
        sq += n.samples[2 * s] * n.samples[2 * s];
        cross += n.samples[2 * s] * n.samples[2 * s + 1];
    }
    const double mean = sum / static_cast<double>(frames);
    const double rms = std::sqrt(sq / static_cast<double>(frames));
    QVERIFY(std::abs(mean) < 0.3e-6);
    QVERIFY(std::abs(rms - 10e-6) < 0.3e-6);
    QVERIFY(std::abs(cross / static_cast<double>(frames)) < 0.03 * 1e-10);

    // Rhythm only: 8 uV at about the requested frequency; its phase drifts, so 10 Hz stays within the alpha band.
    const Drained r = record("synthetic://?realtime=0&duration=2&noise=0&interval=0&stim=0&eeg=1&alpha=10");
    const auto peak = std::max_element(r.samples.begin(), r.samples.end(),
                                       [](float a, float b) { return std::abs(a) < std::abs(b); });
    QVERIFY(std::abs(std::abs(*peak) - 8e-6F) < 0.05e-6F);
    int crossings = 0;
    for (std::size_t s = 1; s < r.samples.size(); ++s) {
        crossings += (r.samples[s - 1] < 0.0F) != (r.samples[s] < 0.0F) ? 1 : 0;
    }
    QVERIFY2(crossings >= 32 && crossings <= 48, qPrintable(QString::number(crossings)));

    // The drift decorrelates the rhythm from the stimuli: averaging over trials removes it, even with an
    // interval of exactly six cycles.
    const Drained locked = record("synthetic://?realtime=0&duration=200&sfreq=100&noise=0&evoked=0&eeg=1&stim=0"
                                  "&interval=0.6&targets=0");
    double residual = 0.0;
    int lags = 0;
    for (int lag = 0; lag < 60; ++lag) {
        const double m = windowMean(locked, 1, 0, 100.0, 1, lag / 100.0, (lag + 1) / 100.0);
        residual += m * m;
        ++lags;
    }
    residual = std::sqrt(residual / lags);
    QVERIFY2(residual < 1.5e-6, qPrintable(QString::number(residual)));
}

//=============================================================================================================

void TestAcqDevice::testSyntheticOddballResponses()
{
    // 400 non-overlapping trials, sampled at 100 Hz so the session fits the ring and no sample is lost. Standards: N100 (-5 uV) and P200 (+4 uV); targets add a P300 (+8 uV at
    // 350 ms). Channel weights follow cos(pi c / n): +1 on the first channel, mirrored on the last.
    const double sfreq = 100.0;
    const Drained d = record("synthetic://?realtime=0&duration=401&sfreq=100&interval=1&seed=3");
    const std::size_t channels = 9;
    QCOMPARE(d.lost, std::uint64_t(0));
    std::size_t targets = 0;
    for (const AcqEvent& e : d.events) {
        targets += e.code == 2 ? 1U : 0U;
    }
    const double share = static_cast<double>(targets) / static_cast<double>(d.events.size());
    QVERIFY(share > 0.14 && share < 0.26);

    const double n100 = windowMean(d, channels, 0, sfreq, 1, 0.090, 0.110);
    const double p200 = windowMean(d, channels, 0, sfreq, 1, 0.190, 0.210);
    const double baseline = windowMean(d, channels, 0, sfreq, 1, -0.2, 0.0);
    const double p300Standard = windowMean(d, channels, 0, sfreq, 1, 0.330, 0.370);
    const double p300Target = windowMean(d, channels, 0, sfreq, 2, 0.330, 0.370);
    QVERIFY2(n100 < -3.5e-6, qPrintable(QString::number(n100)));
    QVERIFY2(p200 > 2.5e-6, qPrintable(QString::number(p200)));
    QVERIFY2(std::abs(baseline) < 1e-6, qPrintable(QString::number(baseline)));
    QVERIFY2(p300Target - p300Standard > 6e-6, qPrintable(QString::number(p300Target - p300Standard)));
    // The last channel sees the mirrored response.
    QVERIFY(windowMean(d, channels, 7, sfreq, 2, 0.330, 0.370) < -4e-6);
}

//=============================================================================================================

void TestAcqDevice::testSyntheticMeg()
{
    auto device = AcqDevice::open("synthetic://?eeg=0&meg=4&stim=off&realtime=false&duration=1&interval=0&alpha=0");
    QVERIFY(device);
    const auto info = device->stream().info();
    QCOMPARE(info->channelCount(), std::size_t(4));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Meg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[0].name, std::string("MEG 001"));
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(device->start().ok());
    const Drained d = drain(*reader);
    double sq = 0.0;
    for (float v : d.samples) {
        sq += static_cast<double>(v) * v;
    }
    const double rms = std::sqrt(sq / static_cast<double>(d.samples.size()));
    QVERIFY2(std::abs(rms - 200e-15) < 15e-15, qPrintable(QString::number(rms)));
}

//=============================================================================================================

void TestAcqDevice::testSyntheticRealtimePacing()
{
    auto device = AcqDevice::open("synthetic://?sfreq=1000&realtime=on&stim=yes");
    QVERIFY(device);
    const auto start = std::chrono::steady_clock::now();
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);
    const std::uint64_t committed = device->stats().streams[0].committed;
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    QVERIFY(device->stop().ok());
    // Paced by the clock: never ahead of real time by more than one block, and not stalled.
    QVERIFY2(committed <= static_cast<std::uint64_t>(elapsedMs + 10), qPrintable(QString::number(committed)));
    QVERIFY2(committed >= 100, qPrintable(QString::number(committed)));
    QCOMPARE(committed % 10, std::uint64_t(0));

    // The clock model places sample 0 on the UTC axis, so a recording gets its measurement date.
    const AcqClockModel clock = device->stream().clock();
    const auto utcNow = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    const std::int64_t sample0Utc = clock.physicalNs(0) + clock.utcOffsetNs;
    QVERIFY2(std::llabs(utcNow - sample0Utc) < 60'000'000'000LL, qPrintable(QString::number(sample0Utc)));

    // speed=20 replays twenty times faster than real time.
    auto fast = AcqDevice::open("synthetic://?sfreq=1000&speed=20");
    QVERIFY(fast);
    const auto fastStart = std::chrono::steady_clock::now();
    QVERIFY(fast->start().ok());
    std::this_thread::sleep_for(200ms);
    const std::uint64_t fastCommitted = fast->stats().streams[0].committed;
    const auto fastMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - fastStart).count();
    QVERIFY(fast->stop().ok());
    QVERIFY2(fastCommitted <= static_cast<std::uint64_t>(20 * fastMs + 10), qPrintable(QString::number(fastCommitted)));
    QVERIFY2(fastCommitted >= 1000, qPrintable(QString::number(fastCommitted)));
}

//=============================================================================================================

void TestAcqDevice::testSyntheticStopRestart()
{
    auto device = AcqDevice::open("synthetic://?sfreq=2000&seed=9");
    QVERIFY(device);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(50ms);
    QVERIFY(device->stop().ok());
    const std::uint64_t first = device->stats().streams[0].committed;
    QVERIFY(device->start().ok());
    // Wait for samples after the restart rather than a fixed time: a loaded CI runner may not schedule the backend.
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (device->stats().streams[0].committed < first + 100 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }
    device->close();

    // The restarted device continues the same signal: identical to an uninterrupted run.
    const Drained resumed = drain(*reader);
    QCOMPARE(resumed.lost, std::uint64_t(0));
    const std::size_t frames = resumed.samples.size() / 9;
    QVERIFY(frames > first);
    const Drained reference = record("synthetic://?sfreq=2000&seed=9&realtime=0&duration=" +
                                     std::to_string(static_cast<double>(frames) / 2000.0));
    QVERIFY(reference.samples.size() >= resumed.samples.size());
    QVERIFY(std::equal(resumed.samples.begin(), resumed.samples.end(), reference.samples.begin()));
}

//=============================================================================================================

void TestAcqDevice::testSyntheticMultiRateStreams()
{
    // EEG at 1 kHz, fNIRS at 10 Hz, ECoG and ERG at 2 kHz: four streams on one clock domain.
    const std::string uri = "synthetic://?eeg=4&fnirs=3&ecog=6&erg=2&realtime=0&duration=4&seed=5&interval=0.7";
    auto device = AcqDevice::open(uri);
    QVERIFY(device);
    QCOMPARE(device->streams().size(), std::size_t(4));
    struct Expected
    {
        const char* name;
        double sfreq;
        std::size_t channels;
        AcqChannelKind kind;
        AcqUnit unit;
        std::uint64_t ticksPerSample;
        std::size_t samples;
    };
    const std::array<Expected, 4> expected = {{{"synthetic", 1000.0, 5, AcqChannelKind::Eeg, AcqUnit::Volt, 2, 4000},
                                               {"fnirs", 10.0, 6, AcqChannelKind::Fnirs, AcqUnit::Molar, 200, 40},
                                               {"ecog", 2000.0, 6, AcqChannelKind::Ecog, AcqUnit::Volt, 1, 8000},
                                               {"erg", 2000.0, 2, AcqChannelKind::Erg, AcqUnit::Volt, 1, 8000}}};
    const AcqTimebase eegClock = device->stream(0).timebase();
    QVERIFY(eegClock.domain != 0);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto info = device->stream(i).info();
        QCOMPARE(info->name, std::string(expected[i].name));
        QCOMPARE(info->sfreq, expected[i].sfreq);
        QCOMPARE(info->channelCount(), expected[i].channels);
        QCOMPARE(info->channels[0].kind, expected[i].kind);
        QCOMPARE(info->channels[0].unit, expected[i].unit);
        const AcqTimebase clock = device->stream(i).timebase();
        QCOMPARE(clock.domain, eegClock.domain);
        QCOMPARE(clock.ticksPerSecond, std::uint64_t(2000));
        QCOMPARE(clock.ticksPerSample, expected[i].ticksPerSample);
        QCOMPARE(clock.lock, AcqLockState::PhaseLocked);
    }
    QCOMPARE(device->stream(1).info()->channels[0].name, std::string("S1_D1 hbo"));
    QCOMPARE(device->stream(1).info()->channels[5].name, std::string("S3_D3 hbr"));
    device->close();

    const std::vector<Drained> streams = recordAll(uri);
    QCOMPARE(streams.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QCOMPARE(streams[i].lost, std::uint64_t(0));
        QCOMPARE(streams[i].samples.size(), expected[i].samples * expected[i].channels);
        QCOMPARE(streams[i].events.size(), streams[0].events.size());
    }
    QVERIFY(streams[0].events.size() >= 4);

    // A trigger lands in every stream on the first sample at or after its instant on the shared clock.
    AcqTimebase from = eegClock;
    for (std::size_t i = 1; i < expected.size(); ++i) {
        AcqTimebase to = from;
        to.ticksPerSample = expected[i].ticksPerSample;
        for (std::size_t e = 0; e < streams[0].events.size(); ++e) {
            const AcqEvent& eeg = streams[0].events[e];
            const AcqEvent& other = streams[i].events[e];
            QCOMPARE(other.code, eeg.code);
            auto mapped = mapSampleExact(from, to, eeg.sample, 0, AcqRounding::Ceil);
            QVERIFY(mapped);
            QCOMPARE(other.sample, *mapped);
            const auto window = mapRangeExact(from, to, eeg.sample, eeg.sample + 1000); // a 1 s epoch
            QVERIFY(window && window->first == other.sample);
            QCOMPARE(window->end - window->first, static_cast<std::uint64_t>(expected[i].sfreq));
        }
    }

    // The extra streams leave the EEG stream untouched.
    const Drained eegOnly = record("synthetic://?eeg=4&realtime=0&duration=4&seed=5&interval=0.7");
    QVERIFY(eegOnly.samples == streams[0].samples);
}

//=============================================================================================================

void TestAcqDevice::testSyntheticMultiRateResponses()
{
    // Noise-free, one stimulus every 15 s, so each response stands alone and the values are exact.
    const std::vector<Drained> d = recordAll(
        "synthetic://?eeg=1&fnirs=2&ecog=10&erg=2&noise=0&alpha=0&realtime=0&duration=75&seed=3&interval=15&targets=0");
    QCOMPARE(d.size(), std::size_t(4));
    QCOMPARE(d[1].events.size(), std::size_t(4));
    for (const Drained& stream : d) {
        QCOMPARE(stream.lost, std::uint64_t(0));
    }

    // fNIRS, 10 Hz: HbO of pair 1 peaks about 5 s after each stimulus, the first at 0.4 uM (later ones sit on the
    // undershoot of the previous response); HbR is -0.3 x HbO.
    const auto at = [](const Drained& s, std::size_t channels, std::uint64_t sample, std::size_t ch) {
        return static_cast<double>(s.samples.at(sample * channels + ch));
    };
    for (const AcqEvent& e : d[1].events) {
        std::uint64_t peak = e.sample;
        for (std::uint64_t s = e.sample; s < e.sample + 120; ++s) {
            peak = at(d[1], 4, s, 0) > at(d[1], 4, peak, 0) ? s : peak;
        }
        QVERIFY2(peak - e.sample >= 45 && peak - e.sample <= 55, qPrintable(QString::number(peak - e.sample)));
        QVERIFY(&e != &d[1].events.front() || std::abs(at(d[1], 4, peak, 0) - 0.4e-6) < 0.01e-6);
        QVERIFY(at(d[1], 4, peak, 0) > 0.3e-6 && at(d[1], 4, peak, 0) < 0.41e-6);
        QVERIFY(std::abs(at(d[1], 4, peak, 1) + 0.3 * at(d[1], 4, peak, 0)) < 1e-12);
        QVERIFY(at(d[1], 4, peak, 2) < at(d[1], 4, peak, 0)); // the second pair lies off the activation
    }

    // ERG, 2 kHz: a-wave around 15 ms, b-wave around 40 ms after the flash.
    for (const AcqEvent& e : d[3].events) {
        std::uint64_t trough = e.sample;
        std::uint64_t peak = e.sample;
        for (std::uint64_t s = e.sample; s < e.sample + 200; ++s) {
            trough = at(d[3], 2, s, 0) < at(d[3], 2, trough, 0) ? s : trough;
            peak = at(d[3], 2, s, 0) > at(d[3], 2, peak, 0) ? s : peak;
        }
        QVERIFY2(trough - e.sample >= 26 && trough - e.sample <= 34, qPrintable(QString::number(trough - e.sample)));
        QVERIFY2(peak - e.sample >= 72 && peak - e.sample <= 90, qPrintable(QString::number(peak - e.sample)));
        QVERIFY(at(d[3], 2, trough, 0) < -50e-6);
        QVERIFY(at(d[3], 2, peak, 0) > 100e-6);
    }

    // ECoG, 2 kHz: high gamma around 150 ms on the focal contacts only; second differences remove the ERP.
    const auto highGamma = [&](std::size_t ch, std::int64_t from, std::int64_t to) {
        double energy = 0.0;
        for (const AcqEvent& e : d[2].events) {
            for (std::int64_t s = static_cast<std::int64_t>(e.sample) + from; s < static_cast<std::int64_t>(e.sample) + to; ++s) {
                const auto u = static_cast<std::uint64_t>(s);
                const double second = at(d[2], 10, u + 1, ch) - 2.0 * at(d[2], 10, u, ch) + at(d[2], 10, u - 1, ch);
                energy += second * second;
            }
        }
        return energy;
    };
    QVERIFY(highGamma(3, 200, 400) > 100.0 * highGamma(3, -400, -200));
    QVERIFY(highGamma(3, 200, 400) > 100.0 * highGamma(9, 200, 400));
}

//=============================================================================================================

/**
 * A live multi-rate session read at the real-time pace, with a stop and restart in the middle: every committed
 * sample of every stream reaches its lossless reader exactly once, and the recording is bit-identical to the same
 * session generated offline.
 */
void TestAcqDevice::testRealtimeRecordingIsComplete()
{
    const std::string params = "eeg=16&fnirs=4&ecog=32&erg=2&seed=21&interval=0.5";
    auto device = AcqDevice::open("synthetic://?" + params + "&speed=20");
    QVERIFY(device);
    const std::size_t streamCount = device->streams().size();
    QCOMPARE(streamCount, std::size_t(4));

    struct Recorder
    {
        AcqReader reader;
        Drained data;
        std::uint64_t gaps = 0;
        std::uint64_t overruns = 0;
    };
    std::vector<Recorder> recorders;
    for (const AcqStream& stream : device->streams()) {
        auto reader = stream.reader(AcqReader::ReadMode::Lossless);
        QVERIFY(reader);
        recorders.push_back({std::move(*reader), {}, 0, 0});
    }
    std::vector<std::thread> threads;
    threads.reserve(recorders.size());
    for (std::size_t i = 0; i < recorders.size(); ++i) {
        threads.emplace_back([&recorder = recorders[i], i] {
            std::uint64_t next = 0;
            std::uint64_t pick = i + 1;
            while (true) {
                pick = pick * 6364136223846793005ULL + 1442695040888963407ULL;
                auto view = recorder.reader.acquire(1, 5s, 1 + static_cast<std::size_t>((pick >> 33U) % 64));
                if (!view) {
                    break;
                }
                recorder.gaps += view->firstSample() != next || view->lost() != 0 ? 1U : 0U;
                for (std::size_t p = 0; p < view->partCount(); ++p) {
                    const AcqBlockPart& part = view->part(p);
                    for (std::size_t s = 0; s < part.samples; ++s) {
                        for (std::size_t c = 0; c < view->channelCount(); ++c) {
                            float v = 0.0F;
                            std::memcpy(&v, part.at(c, s), sizeof(v));
                            recorder.data.samples.push_back(v);
                        }
                    }
                }
                for (std::size_t e = 0; e < view->events().size(); ++e) {
                    recorder.data.events.push_back(view->events()[e]);
                }
                next = view->firstSample() + view->sampleCount();
                recorder.overruns += recorder.reader.release(*view).ok() ? 0U : 1U;
            }
        });
    }

    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);
    QVERIFY(device->stop().ok());
    std::this_thread::sleep_for(50ms);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(500ms);
    const AcqDeviceStats stats = device->stats();
    device->close();
    for (auto& thread : threads) {
        thread.join();
    }

    // Streams outlive the closed device through their readers.
    const std::uint64_t eegSamples = recorders[0].reader.stream().stats().committed;
    QVERIFY2(eegSamples >= 10000, qPrintable(QString::number(eegSamples))); // about 16 s of session at speed 20
    const std::vector<Drained> offline =
        recordAll("synthetic://?" + params + "&realtime=0&duration=" + std::to_string(static_cast<double>(eegSamples) / 1000.0));
    QCOMPARE(offline.size(), streamCount);
    for (std::size_t i = 0; i < streamCount; ++i) {
        const Recorder& r = recorders[i];
        const AcqReaderStats readerStats = r.reader.stats();
        const AcqStream stream = r.reader.stream();
        const std::uint64_t committed = stream.stats().committed;
        QCOMPARE(r.gaps, std::uint64_t(0));
        QCOMPARE(r.overruns, std::uint64_t(0));
        QCOMPARE(readerStats.lost, std::uint64_t(0));
        QCOMPARE(readerStats.eventsLost, std::uint64_t(0));
        QCOMPARE(readerStats.delivered, committed);
        QVERIFY(stats.streams[i].committed <= committed);
        QCOMPARE(r.data.samples.size(), committed * stream.info()->channelCount());
        QVERIFY2(r.data.samples == offline[i].samples, stream.info()->name.c_str());
        QCOMPARE(r.data.events.size(), offline[i].events.size());
        for (std::size_t e = 0; e < r.data.events.size(); ++e) {
            QCOMPARE(r.data.events[e].sample, offline[i].events[e].sample);
            QCOMPARE(r.data.events[e].code, offline[i].events[e].code);
        }
    }
}

//=============================================================================================================

void TestAcqDevice::testSyntheticInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("key");
    QTest::newRow("no channels") << "synthetic://?eeg=0&meg=0&stim=0" << "eeg";
    QTest::newRow("negative count") << "synthetic://?eeg=-1" << "eeg";
    QTest::newRow("not a number") << "synthetic://?sfreq=fast" << "sfreq";
    QTest::newRow("zero rate") << "synthetic://?sfreq=0" << "sfreq";
    QTest::newRow("trailing text") << "synthetic://?sfreq=1000Hz" << "sfreq";
    QTest::newRow("bool") << "synthetic://?realtime=maybe" << "realtime";
    QTest::newRow("speed") << "synthetic://?speed=0" << "speed";
    QTest::newRow("negative interval") << "synthetic://?interval=-1" << "interval";
    QTest::newRow("block zero") << "synthetic://?block=0" << "block";
    QTest::newRow("target share") << "synthetic://?targets=1.5" << "targets";
    QTest::newRow("target") << "synthetic://host" << "host";
    QTest::newRow("alpha above Nyquist") << "synthetic://?sfreq=100&alpha=50" << "alpha";
    QTest::newRow("interval below two pulses") << "synthetic://?interval=0.005" << "interval";
    QTest::newRow("fNIRS below cardiac Nyquist") << "synthetic://?fnirs=2&fnirs_sfreq=2" << "fnirs_sfreq";
    QTest::newRow("fractional fNIRS rate") << "synthetic://?fnirs=2&fnirs_sfreq=7.8125" << "fnirs_sfreq";
    QTest::newRow("fractional EEG rate with ECoG") << "synthetic://?sfreq=512.5&ecog=4" << "sfreq";
    QTest::newRow("ECoG below high gamma") << "synthetic://?ecog=4&ecog_sfreq=200" << "ecog_sfreq";
    QTest::newRow("ERG below oscillatory potentials") << "synthetic://?erg=2&erg_sfreq=250" << "erg_sfreq";
    QTest::newRow("negative ERG count") << "synthetic://?erg=-2" << "erg";
}

void TestAcqDevice::testSyntheticInvalidParameters()
{
    QFETCH(QString, uri);
    QFETCH(QString, key);
    auto device = AcqDevice::open(uri.toStdString());
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::InvalidParameter);
    QVERIFY2(device.error().context().find(key.toStdString()) != std::string_view::npos,
             std::string(device.error().context()).c_str());
}

//=============================================================================================================


//=============================================================================================================

void TestAcqDevice::testBackpressureReplayIsLossless()
{
    // Generated far faster than the slow readers consume; only the producer waiting keeps every sample.
    const std::string uri = "synthetic://?eeg=4&fnirs=2&ecog=6&erg=2&realtime=0&duration=150&seed=5&interval=0.5";
    auto device = AcqDevice::open(uri);
    QVERIFY(device);
    auto lossy = device->stream(0).reader(AcqReader::ReadMode::Lossless);
    QVERIFY(lossy && device->start().ok());
    QTRY_COMPARE_WITH_TIMEOUT(lossy->stream().stats().committed, std::uint64_t(150000), 30000);
    const std::uint64_t capacity = lossy->stream().stats().capacitySamples;
    QCOMPARE(drain(*lossy).lost, std::uint64_t(150000) - capacity);
    device->close();

    // Readers that start only once the producer is held up by a full ring.
    auto device2 = AcqDevice::open(uri);
    QVERIFY(device2);
    AcqReaderOptions gating;
    gating.gating = true;
    std::vector<AcqReader> readers;
    for (const AcqStream& stream : device2->streams()) {
        auto reader = stream.reader(AcqReader::ReadMode::Lossless, gating);
        QVERIFY(reader);
        readers.push_back(std::move(*reader));
    }
    QVERIFY(device2->start().ok());
    auto waits = [&readers] {
        std::uint64_t n = 0;
        for (const AcqReader& r : readers) {
            n += r.stream().stats().writerWaits;
        }
        return n;
    };
    QTRY_VERIFY_WITH_TIMEOUT(waits() > 0, 10000);
    std::vector<Drained> slow(readers.size());
    std::vector<std::thread> threads;
    threads.reserve(readers.size());
    for (std::size_t i = 0; i < readers.size(); ++i) {
        threads.emplace_back([&slow, &readers, i] { slow[i] = drain(readers[i], 512); });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    const std::vector<Drained> fast = recordAll(uri);
    QCOMPARE(slow.size(), std::size_t(4));
    QCOMPARE(fast.size(), std::size_t(4));
    QCOMPARE(slow[0].samples.size(), std::size_t(150000 * 5)); // 4 EEG + STI 014
    for (std::size_t i = 0; i < slow.size(); ++i) {
        QCOMPARE(slow[i].lost, std::uint64_t(0));
        QCOMPARE(fast[i].lost, std::uint64_t(0));
        QVERIFY(!slow[i].samples.empty() && !slow[i].events.empty());
        QVERIFY(slow[i].samples == fast[i].samples);
        QCOMPARE(slow[i].events.size(), fast[i].events.size());
    }

    // A gating reader that never reads holds the session, and stop() still ends the wait.
    auto held = AcqDevice::open(uri);
    QVERIFY(held);
    auto idle = held->stream(0).reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(idle && held->start().ok());
    const std::size_t filled = idle->stream().stats().capacitySamples / 10 * 10; // whole 10-sample blocks
    QTRY_COMPARE_WITH_TIMEOUT(idle->available(), filled, 10000);
    std::this_thread::sleep_for(50ms);
    QCOMPARE(idle->available(), filled);
    QVERIFY(held->stop().ok());
    QCOMPARE(idle->available(), filled);
    QCOMPARE(idle->stats().lost, std::uint64_t(0));
}

//=============================================================================================================

void TestAcqDevice::testImpedances()
{
    const std::string uri = "synthetic://?eeg=6&meg=2&ecog=3&erg=2&fnirs=1&seed=4";
    auto device = AcqDevice::open(uri);
    QVERIFY(device);
    auto z = device->measureImpedances(1s);
    QVERIFY(z);
    // Electrodes only (6 EEG, 3 ECoG, 2 ERG), in stream and channel order, each in the range of its kind.
    QCOMPARE(z->size(), std::size_t(11));
    std::size_t previous = 0;
    for (std::size_t i = 0; i < z->size(); ++i) {
        const AcqImpedance& e = (*z)[i];
        const AcqChannelKind kind = device->stream(e.stream).info()->channels[e.channel].kind;
        const auto [lo, hi] = kind == AcqChannelKind::Eeg ? std::pair{2e3, 20e3}
            : kind == AcqChannelKind::Ecog                ? std::pair{1e3, 5e3}
                                                          : std::pair{5e3, 15e3};
        QVERIFY(kind == AcqChannelKind::Eeg || kind == AcqChannelKind::Ecog || kind == AcqChannelKind::Erg);
        QVERIFY(e.ohms >= lo && e.ohms < hi);
        const std::size_t key = e.stream * 1000 + e.channel;
        QVERIFY(i == 0 || key > previous);
        previous = key;
    }
    // Seeded: the same electrodes give the same values, another seed others.
    auto same = AcqDevice::open(uri)->measureImpedances();
    auto other = AcqDevice::open("synthetic://?eeg=6&meg=2&ecog=3&erg=2&fnirs=1&seed=5")->measureImpedances();
    QVERIFY(same && other);
    QCOMPARE((*same)[0].ohms, (*z)[0].ohms);
    QVERIFY((*other)[0].ohms != (*z)[0].ohms);

    // Not while running; again once stopped; never after close.
    auto reader = device->stream().reader(AcqReader::ReadMode::Latest);
    QVERIFY(device->start().ok());
    QVERIFY(device->measureImpedances().error() == AcqErrorCode::InvalidState);
    QVERIFY(device->stop().ok());
    QVERIFY(device->measureImpedances());
    device->close();
    QVERIFY(device->measureImpedances().error() == AcqErrorCode::InvalidState);
    QVERIFY(AcqDevice().measureImpedances().error() == AcqErrorCode::InvalidState);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqDevice)
#include "test_acq_device.moc"
