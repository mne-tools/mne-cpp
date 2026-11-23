//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Live multi-channel, multi-rate scope of a streaming source with a closed-loop stimulator on the same clock.
 *
 * Opens an acquisition source by URI (default: the seeded simulator with 8 EEG channels and a trigger channel at
 * 1 kHz, 2 fNIRS pairs at 10 Hz, 8 ECoG contacts and 2 ERG electrodes at 2 kHz) and sweeps every channel of every
 * stream across the same 5 s window as the samples arrive, like the monitor of an amplifier, with the stimulus
 * triggers marked. Each stream keeps its own rate: the slow fNIRS trace advances in 100 ms steps, the ECoG and ERG
 * traces at twice the EEG rate, all aligned in time because the streams share one clock. Each view is converted to
 * physical units with decode(), whatever the device's format and layout.
 *
 * A closed-loop stimulator answers every trigger with a biphasic current pulse 200 ms later (1 mA after targets,
 * 0.5 mA after standards). It writes the pulse into an output stream with beginWriteAt() and encode(); an emulated
 * DAC plays that stream on the input's clock and the bottom trace shows what it played. The output ring is kept
 * small, so many pulses cross its end and are written in two parts (marked "wrap"): every played sample is checked
 * against the scheduled pulse, so a pulse that is truncated, shifted or written wrongly across the end of the ring
 * shows up in the trace and in the mismatch count.
 *
 * Run it without arguments to watch the simulator, with a URI to watch another source, or with
 * --seconds <s> to stop after s seconds (it then exits non-zero if any played sample was wrong).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq.h>
#include <mne/disp/plots/helpers/figureexporter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QFont>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPolygonF>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;
using namespace std::chrono_literals;

namespace
{

constexpr double kWindowSeconds = 5.0;
constexpr double kDelaySeconds = 0.2;
constexpr double kPhaseSeconds = 0.06; // per phase of the biphasic pulse
constexpr double kMaxViewSeconds = 0.1;
constexpr double kFullScale = 2e-3; // A, output range of the stimulator
constexpr std::int32_t kStandard = 1;
constexpr std::int32_t kTarget = 2;

struct Marker
{
    std::uint64_t sample = 0;
    std::int32_t code = 0;
    bool wrapped = false; // pulse markers: written in two parts across the end of the ring
};

// Colour scheme (Tokyo Night): dark background, one muted hue per modality, red/blue for HbO/HbR as in fNIRS.
const QColor kBackground(0x1a, 0x1b, 0x26);
const QColor kSeparator(0x29, 0x2e, 0x42);
const QColor kLabel(0xc0, 0xca, 0xf5);
const QColor kScale(0x73, 0x7a, 0xa2);
const QColor kTitle(0x7a, 0xa2, 0xf7);
const QColor kStimulus(0xff, 0x9e, 0x64);
const QColor kStandardMarker(0x7a, 0xa2, 0xf7, 110);
const QColor kTargetMarker(0xf7, 0x76, 0x8e, 150);
const QColor kCursor(0xc0, 0xca, 0xf5, 200);

/** How a channel is drawn: trace colour and SI unit symbol. */
struct ChannelStyle
{
    QColor colour;
    QString unit;
};

ChannelStyle channelStyle(const AcqChannelInfo& channel)
{
    QString unit;
    switch (channel.unit) {
        case AcqUnit::Volt:
            unit = "V";
            break;
        case AcqUnit::Tesla:
            unit = "T";
            break;
        case AcqUnit::Ampere:
            unit = "A";
            break;
        case AcqUnit::Molar:
            unit = "M";
            break;
        default:
            break;
    }
    switch (channel.kind) {
        case AcqChannelKind::Eeg:
            return {QColor(0x7a, 0xa2, 0xf7), unit};
        case AcqChannelKind::Meg:
        case AcqChannelKind::Opm:
            return {QColor(0xbb, 0x9a, 0xf7), unit};
        case AcqChannelKind::Ecog:
        case AcqChannelKind::Seeg:
            return {QColor(0x9e, 0xce, 0x6a), unit};
        case AcqChannelKind::Erg:
            return {QColor(0xe0, 0xaf, 0x68), unit};
        case AcqChannelKind::Fnirs:
            return {channel.name.ends_with("hbr") ? QColor(0x7d, 0xcf, 0xff) : QColor(0xf7, 0x76, 0x8e), unit};
        case AcqChannelKind::StimCurrent:
        case AcqChannelKind::StimVoltage:
            return {kStimulus, unit};
        default:
            return {QColor(0xa9, 0xb1, 0xd6), unit};
    }
}

/** A sensitivity as "±50 µV": the SI prefix that puts the number between 1 and 1000. */
QString formatRange(double value, const QString& unit)
{
    if (unit.isEmpty()) {
        return QString("±%1").arg(value, 0, 'g', 3);
    }
    static const std::array<std::pair<double, const char*>, 6> kPrefixes = {
        {{1e-15, "f"}, {1e-12, "p"}, {1e-9, "n"}, {1e-6, "µ"}, {1e-3, "m"}, {1.0, ""}}};
    auto prefix = kPrefixes.back();
    for (const auto& candidate : kPrefixes) {
        if (value >= candidate.first * 0.9995) {
            prefix = candidate;
        }
    }
    return QString("±%1 %2%3").arg(value / prefix.first, 0, 'g', 3).arg(QString::fromUtf8(prefix.second), unit);
}

/** Rolling history of one stream in physical units. */
struct Trace
{
    std::string title;
    double sfreq = 0.0;
    std::uint64_t window = 0;   // samples per sweep
    std::size_t rows = 0;       // channels, on the first stream plus the played-back stimulator
    std::size_t length = 0;     // history length, a power of two above the window
    std::vector<float> history; // frames of `rows` values, sample s at (s & (length - 1))
    std::uint64_t newest = 0;   // one past the newest sample in the history
    std::uint64_t oldest = 0;   // first valid sample in the history
    StringList names;
    std::vector<ChannelStyle> styles;

    Trace(const AcqStreamInfo& info, std::size_t extraRows)
    : title(QString("%1  %2 Hz").arg(QString::fromStdString(info.name)).arg(info.sfreq).toStdString())
    , sfreq(info.sfreq)
    , window(static_cast<std::uint64_t>(std::llround(kWindowSeconds * info.sfreq)))
    , rows(info.channelCount() + extraRows)
    , length(std::bit_ceil(static_cast<std::size_t>(window) + 1))
    , history(length * rows, 0.0F)
    {
        for (const AcqChannelInfo& channel : info.channels) {
            names.push_back(channel.name);
            styles.push_back(channelStyle(channel));
        }
    }

    /** Appends @p n frames of @p channels values, plus one value per frame from @p extra if given. */
    void append(std::uint64_t first, std::size_t n, const float* frames, std::size_t channels, const float* extra)
    {
        if (first > newest) {
            oldest = first; // a gap: never show the stale samples in between
        }
        for (std::size_t i = 0; i < n; ++i) {
            float* frame = &history[((first + i) & (length - 1)) * rows];
            std::memcpy(frame, frames + i * channels, channels * sizeof(float));
            if (extra != nullptr) {
                frame[channels] = extra[i];
            }
        }
        newest = first + n;
        oldest = std::max(oldest, newest > length ? newest - length : 0);
    }
};

/** Everything the acquisition thread hands to the display, guarded by one mutex. */
struct Shared
{
    std::mutex mutex;
    std::vector<Trace> traces; // the first stream (with the stimulator output) first, then the device's others
    std::deque<Marker> triggers;
    std::deque<Marker> pulses;

    std::atomic<std::uint64_t> lost{0};
    std::atomic<std::uint64_t> views{0};
    std::atomic<std::uint64_t> wrappedViews{0};
    std::atomic<std::uint64_t> pulsesScheduled{0};
    std::atomic<std::uint64_t> pulsesWrapped{0};
    std::atomic<std::uint64_t> pulsesRefused{0};
    std::atomic<std::uint64_t> playedMismatches{0};
    std::atomic<bool> stop{false};
};

/** Biphasic pulse: cathodic first, then anodic. */
double pulseValue(std::uint64_t phase, std::size_t phaseSamples, double amplitude)
{
    return phase < phaseSamples ? -amplitude : amplitude;
}

struct Scheduled
{
    std::uint64_t at = 0;
    double amplitude = 0.0;
};

/** Reads everything @p reader has now into trace @p index; used for the device's further streams. */
void drainInto(AcqReader& reader, std::size_t index, std::vector<float>& block, Shared& shared)
{
    const std::size_t channels = reader.channelCount();
    const std::size_t maxView = block.size() / channels;
    while (true) {
        auto view = reader.acquire(0, 0ns, maxView);
        if (!view) {
            return;
        }
        const std::size_t n = view->sampleCount();
        const std::uint64_t first = view->firstSample();
        shared.lost.fetch_add(view->lost(), std::memory_order_relaxed);
        const bool decoded = n > 0 && decode(*view, AcqMatrixRef<float>::columnMajor(block.data(), channels, n)).ok();
        (void)reader.release(*view);
        if (!decoded) {
            return;
        }
        std::lock_guard<std::mutex> lock(shared.mutex);
        shared.traces[index].append(first, n, block.data(), channels, nullptr);
    }
}

/** Acquisition thread: reads the source, schedules pulses, plays the output ring, fills the histories. */
void acquire(AcqReader& input, std::vector<AcqReader>& others, AcqWriter& stimulator, AcqReader& dac, Shared& shared)
{
    const std::size_t channels = input.channelCount();
    const double sfreq = shared.traces[0].sfreq;
    const auto delay = static_cast<std::uint64_t>(std::llround(kDelaySeconds * sfreq));
    const auto phaseSamples = static_cast<std::size_t>(std::llround(kPhaseSeconds * sfreq));
    const std::size_t pulseSamples = 2 * phaseSamples;
    const auto maxView = static_cast<std::size_t>(std::llround(kMaxViewSeconds * sfreq));
    const double lsb = kFullScale / 32767.0;

    std::vector<float> block(maxView * channels);
    std::vector<float> played(maxView);
    std::vector<double> pulse(pulseSamples);
    std::deque<Scheduled> scheduled;
    std::uint64_t next = 0; // next input sample the DAC has to play
    std::vector<std::vector<float>> otherBlocks;
    for (std::size_t k = 0; k < others.size(); ++k) {
        const auto maxOther = std::max<std::size_t>(1, static_cast<std::size_t>(kMaxViewSeconds * shared.traces[k + 1].sfreq));
        otherBlocks.emplace_back(maxOther * others[k].channelCount());
    }

    while (!shared.stop.load(std::memory_order_relaxed)) {
        auto view = input.acquire(1, 100ms, maxView);
        if (!view) {
            if (view.error() == AcqErrorCode::EndOfStream) {
                break;
            }
            continue;
        }
        const std::uint64_t first = view->firstSample();
        const std::size_t n = view->sampleCount();
        shared.views.fetch_add(1, std::memory_order_relaxed);
        shared.wrappedViews.fetch_add(view->partCount() > 1 ? 1U : 0U, std::memory_order_relaxed);
        shared.lost.fetch_add(view->lost(), std::memory_order_relaxed);
        if (first > next) {
            (void)dac.consumeIdle(static_cast<std::size_t>(first - next)); // samples lost on input: play silence
        }
        next = first + n;

        if (const AcqError error = decode(*view, AcqMatrixRef<float>::columnMajor(block.data(), channels, n));
            !error.ok()) {
            std::fprintf(stderr, "Cannot decode: %s\n", error.text().c_str());
            (void)input.release(*view);
            break;
        }

        std::vector<Marker> newTriggers;
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            const AcqEvent& event = view->events()[e];
            if (event.code != kStandard && event.code != kTarget) {
                continue;
            }
            newTriggers.push_back({event.sample, event.code, false});

            //! [acq_scope_stimulate]
            // Answer the trigger with a biphasic pulse on the exact output sample, delay samples later.
            const std::uint64_t at = event.sample + delay;
            const double amplitude = event.code == kTarget ? 1e-3 : 0.5e-3;
            auto write = stimulator.beginWriteAt(at, pulseSamples, 0ns);
            if (!write) {
                shared.pulsesRefused.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            for (std::size_t j = 0; j < pulseSamples; ++j) {
                pulse[j] = pulseValue(write->firstSample() + j - at, phaseSamples, amplitude);
            }
            (void)encode(AcqMatrixRef<const double>::rowMajor(pulse.data(), 1, pulseSamples), *write);
            const bool wrapped = write->partCount() > 1;
            stimulator.commit(pulseSamples);
            //! [acq_scope_stimulate]
            scheduled.push_back({at, amplitude});
            shared.pulsesScheduled.fetch_add(1, std::memory_order_relaxed);
            shared.pulsesWrapped.fetch_add(wrapped ? 1U : 0U, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(shared.mutex);
            shared.pulses.push_back({at, event.code, wrapped});
        }
        (void)input.release(*view);

        // The DAC plays the output samples of the same instants on the same clock.
        std::fill(played.begin(), played.begin() + static_cast<std::ptrdiff_t>(n), 0.0F);
        std::size_t got = 0;
        if (auto out = dac.acquire(0, 0ns, n)) {
            got = out->sampleCount();
            if (got > 0) {
                (void)decode(*out, AcqMatrixRef<float>::rowMajor(played.data(), 1, got));
            }
            (void)dac.release(*out);
        }
        if (got < n) {
            (void)dac.consumeIdle(n - got);
        }
        while (!scheduled.empty() && scheduled.front().at + pulseSamples <= first) {
            scheduled.pop_front();
        }
        std::uint64_t mismatches = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint64_t t = first + i;
            double expected = 0.0;
            for (const Scheduled& p : scheduled) {
                if (t >= p.at && t < p.at + pulseSamples) {
                    expected = pulseValue(t - p.at, phaseSamples, p.amplitude);
                }
            }
            mismatches += std::abs(static_cast<double>(played[i]) - expected) > lsb ? 1U : 0U;
        }
        shared.playedMismatches.fetch_add(mismatches, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(shared.mutex);
            Trace& trace = shared.traces[0];
            trace.append(first, n, block.data(), channels, played.data());
            shared.triggers.insert(shared.triggers.end(), newTriggers.begin(), newTriggers.end());
            const std::uint64_t windowStart = trace.newest > trace.length ? trace.newest - trace.length : 0;
            while (!shared.triggers.empty() && shared.triggers.front().sample < windowStart) {
                shared.triggers.pop_front();
            }
            while (!shared.pulses.empty() && shared.pulses.front().sample < windowStart) {
                shared.pulses.pop_front();
            }
        }
        for (std::size_t k = 0; k < others.size(); ++k) {
            drainInto(others[k], k + 1, otherBlocks[k], shared);
        }
    }
}

//=============================================================================================================
/**
 * Sweep display, drawn like an oscilloscope: each channel runs left to right and is overwritten at the cursor,
 * every stream at its own rate across the same 5 s. Each stream keeps an image of its rows; a frame folds the new
 * samples into per-column minima and maxima and repaints only the columns the cursor passed, so a frame costs the
 * same however many samples the window holds. The image is redrawn whole when a sweep starts (new sensitivity),
 * after a resize, and for streams with fewer samples than columns, which are drawn through their samples.
 */
class ScopeView : public QWidget
{
public:
    explicit ScopeView(Shared& shared)
    : m_shared(shared)
    {
        m_labelFont = font();
        m_labelFont.setPointSizeF(m_labelFont.pointSizeF() * 0.95);
        m_scaleFont = font();
        m_scaleFont.setPointSizeF(m_scaleFont.pointSizeF() * 0.85);
        setMinimumSize(900, 600);
        setAutoFillBackground(true);
        QPalette p = palette();
        p.setColor(QPalette::Window, kBackground);
        setPalette(p);
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        const int plotWidth = std::max(1, width() - kLabelWidth - 8);
        {
            // Hold the lock only while folding the new samples into the column envelopes.
            std::lock_guard<std::mutex> lock(m_shared.mutex);
            if (m_lanes.size() != m_shared.traces.size()) {
                m_lanes.assign(m_shared.traces.size(), Lane());
            }
            for (std::size_t t = 0; t < m_shared.traces.size(); ++t) {
                fold(m_lanes[t], m_shared.traces[t], plotWidth);
            }
            m_triggers.assign(m_shared.triggers.begin(), m_shared.triggers.end());
            m_pulses.assign(m_shared.pulses.begin(), m_shared.pulses.end());
        }

        QPainter painter(this);
        const Lane& base = m_lanes[0];
        if (base.newest == 0) {
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, "waiting for samples ...");
            return;
        }
        std::size_t rows = 0;
        for (const Lane& lane : m_lanes) {
            rows += lane.rows;
        }
        const double rowHeight = static_cast<double>(height() - 20) / static_cast<double>(rows);

        double top = 10.0;
        double outTop = 0.0;
        for (std::size_t t = 0; t < m_lanes.size(); ++t) {
            Lane& lane = m_lanes[t];
            render(lane, t == 0, rowHeight);
            painter.drawImage(QPointF(0, top), lane.image);
            if (t == 0) {
                outTop = top + static_cast<double>(lane.rows - 1) * rowHeight;
            }
            top += static_cast<double>(lane.rows) * rowHeight;
        }

        // Triggers across all streams, pulses on the output trace; the cursor marks the newest sample.
        const std::uint64_t newest = base.newest;
        const std::uint64_t window = base.window;
        auto xOf = [&](std::uint64_t phase) {
            return kLabelWidth + static_cast<double>(phase) * plotWidth / static_cast<double>(window);
        };
        for (const Marker& m : m_triggers) {
            if (m.sample + window < newest) {
                continue;
            }
            const double x = xOf(m.sample % window);
            painter.setPen(QPen(m.code == kTarget ? kTargetMarker : kStandardMarker, 1, Qt::DashLine));
            painter.drawLine(QPointF(x, 10), QPointF(x, height() - 10));
            painter.drawText(QPointF(x + 3, 22), m.code == kTarget ? "T" : "S");
        }
        painter.setPen(kStimulus);
        for (const Marker& m : m_pulses) {
            if (m.wrapped && m.sample + window >= newest && m.sample < newest) {
                painter.drawText(QPointF(xOf(m.sample % window) + 2, outTop + 12), "wrap");
            }
        }
        painter.setPen(QPen(kCursor, 2));
        const double cursor = xOf(newest % window);
        painter.drawLine(QPointF(cursor, 4), QPointF(cursor, height() - 4));
        painter.setPen(kScale);
        painter.drawText(QRectF(kLabelWidth, height() - 18, plotWidth, 16), Qt::AlignRight,
                         QString("t = %1 s, %2 s per sweep")
                             .arg(static_cast<double>(newest) / base.sfreq, 0, 'f', 1)
                             .arg(kWindowSeconds, 0, 'f', 0));
    }

private:
    static constexpr int kLabelWidth = 150;
    static constexpr std::uint64_t kEmpty = std::numeric_limits<std::uint64_t>::max();

    /** Minimum and maximum of one column of one row, with the sweep phase of each, and the sweep they belong to. */
    struct Column
    {
        float low = 0.0F;
        float high = 0.0F;
        std::uint64_t lowPhase = 0;
        std::uint64_t highPhase = 0;
        std::uint64_t sweep = kEmpty;
    };

    /** A stream as drawn: column envelopes (dense) or the samples of the window (sparse), and its image. */
    struct Lane
    {
        std::string title;
        double sfreq = 0.0;
        std::uint64_t window = 0;
        std::size_t rows = 0;
        StringList names;
        std::vector<ChannelStyle> styles;
        int width = 0;
        bool dense = false;
        std::uint64_t newest = 0;          // samples folded in so far
        std::vector<Column> columns;       // dense: rows x width
        std::vector<float> samples;        // sparse: rows x window, by sweep phase
        std::vector<std::uint64_t> sweeps; // sparse: sweep of each phase
        std::vector<float> ranges;         // per row, held for a sweep
        QImage image;                      // labels and traces of the rows
        double rowHeight = 0.0;
        std::uint64_t painted = 0; // newest sample in the image
        std::uint64_t paintedSweep = kEmpty;
    };

    static std::size_t columnOf(std::uint64_t phase, std::uint64_t window, int width)
    {
        return static_cast<std::size_t>(phase * static_cast<std::uint64_t>(width) / window);
    }

    /** Folds the samples of @p trace that are new since the last frame into @p lane. Called under the lock. */
    static void fold(Lane& lane, const Trace& trace, int width)
    {
        if (lane.width != width || lane.window != trace.window || lane.rows != trace.rows) {
            lane = Lane();
            lane.title = trace.title;
            lane.sfreq = trace.sfreq;
            lane.window = trace.window;
            lane.rows = trace.rows;
            lane.names = trace.names;
            lane.styles = trace.styles;
            lane.width = width;
            lane.dense = trace.window >= static_cast<std::uint64_t>(width);
            if (lane.dense) {
                lane.columns.assign(lane.rows * static_cast<std::size_t>(width), Column());
            } else {
                lane.samples.assign(lane.rows * trace.window, 0.0F);
                lane.sweeps.assign(trace.window, kEmpty);
            }
            lane.ranges.assign(lane.rows, 0.0F);
        }
        if (trace.newest > lane.newest + trace.window || trace.oldest > lane.newest) {
            lane.paintedSweep = kEmpty; // samples were skipped: repaint from the envelopes
        }
        // Only what the history still holds and the window still shows.
        const std::uint64_t floor = trace.newest > trace.window ? trace.newest - trace.window : 0;
        const std::uint64_t from = std::max({lane.newest, trace.oldest, floor});
        for (std::uint64_t s = from; s < trace.newest; ++s) {
            const std::uint64_t phase = s % trace.window;
            const std::uint64_t sweep = s / trace.window;
            const float* frame = &trace.history[(s & (trace.length - 1)) * trace.rows];
            if (lane.dense) {
                const std::size_t x = columnOf(phase, trace.window, width);
                for (std::size_t r = 0; r < lane.rows; ++r) {
                    Column& c = lane.columns[r * static_cast<std::size_t>(width) + x];
                    const float v = frame[r];
                    if (c.sweep != sweep) {
                        c = {v, v, phase, phase, sweep};
                    } else if (v < c.low) {
                        c.low = v;
                        c.lowPhase = phase;
                    } else if (v > c.high) {
                        c.high = v;
                        c.highPhase = phase;
                    }
                }
            } else {
                for (std::size_t r = 0; r < lane.rows; ++r) {
                    lane.samples[r * trace.window + phase] = frame[r];
                }
                lane.sweeps[phase] = sweep;
            }
        }
        lane.newest = trace.newest;
    }

    /** Whether data of @p sweep at @p phase is on screen: this sweep left of the cursor, the last one right of it. */
    static bool visible(const Lane& lane, std::uint64_t sweep, std::uint64_t phase)
    {
        const std::uint64_t current = lane.newest / lane.window;
        const std::uint64_t cursor = lane.newest % lane.window;
        return sweep == current ? phase < cursor : sweep + 1 == current && phase >= cursor;
    }

    /**
     * Appends the points of row @p r over columns [x0, x1] to m_points in time order and in raw units, starting
     * a new polyline wherever the trace is not continuous. @return The largest magnitude among them.
     */
    float collect(const Lane& lane, std::size_t r, std::size_t x0, std::size_t x1)
    {
        float rangeMax = 0.0F;
        std::uint64_t previous = kEmpty;
        auto add = [&](double x, float v, std::uint64_t sweep) {
            if (sweep != previous) {
                m_breaks.push_back(m_points.size()); // never join the new sweep to the old one across the cursor
                previous = sweep;
            }
            m_points.emplace_back(x, static_cast<double>(v));
            rangeMax = std::max(rangeMax, std::abs(v));
        };
        if (lane.dense) {
            const auto width = static_cast<std::size_t>(lane.width);
            for (std::size_t x = x0; x <= x1; ++x) {
                const Column& c = lane.columns[r * width + x];
                if (c.sweep == kEmpty || !visible(lane, c.sweep, c.lowPhase)) {
                    previous = kEmpty;
                    continue;
                }
                // The extremes in time order, so an edge is one stroke instead of a down-up-down zig-zag.
                const bool lowFirst = c.lowPhase <= c.highPhase;
                add(kLabelWidth + static_cast<double>(x), lowFirst ? c.low : c.high, c.sweep);
                add(kLabelWidth + static_cast<double>(x), lowFirst ? c.high : c.low, c.sweep);
            }
        } else {
            for (std::uint64_t phase = 0; phase < lane.window; ++phase) {
                if (!visible(lane, lane.sweeps[phase], phase)) {
                    previous = kEmpty;
                    continue;
                }
                add(kLabelWidth + static_cast<double>(phase) * lane.width / static_cast<double>(lane.window),
                    lane.samples[r * lane.window + phase], lane.sweeps[phase]);
            }
        }
        return rangeMax;
    }

    /** Brings the image of @p lane up to date: whole at a sweep start, otherwise only the new columns. */
    void render(Lane& lane, bool withOutput, double rowHeight)
    {
        const std::uint64_t sweep = lane.newest / lane.window;
        const qreal dpr = devicePixelRatioF();
        const QSize size(width(), static_cast<int>(std::ceil(static_cast<double>(lane.rows) * rowHeight)));
        const QSize pixels(static_cast<int>(std::ceil(size.width() * dpr)), static_cast<int>(std::ceil(size.height() * dpr)));
        const bool geometry = lane.paintedSweep != sweep || lane.rowHeight != rowHeight || lane.image.size() != pixels;
        if (!geometry && lane.painted == lane.newest) {
            return;
        }
        const bool full = geometry || !lane.dense;
        std::size_t x0 = 0;
        std::size_t x1 = static_cast<std::size_t>(lane.width) - 1;
        if (full) {
            if (lane.image.size() != pixels) {
                lane.image = QImage(pixels, QImage::Format_RGB32); // opaque: the cheapest image to blit
                lane.image.setDevicePixelRatio(dpr);
            }
            lane.image.fill(kBackground);
            lane.rowHeight = rowHeight;
        } else {
            // From the column of the last painted sample (it may have gained samples) to the newest one.
            x0 = columnOf((lane.painted == 0 ? 0 : lane.painted - 1) % lane.window, lane.window, lane.width);
            x1 = columnOf((lane.newest - 1) % lane.window, lane.window, lane.width);
        }

        QPainter painter(&lane.image);
        painter.setRenderHint(QPainter::Antialiasing, false);
        if (!full) {
            painter.fillRect(QRectF(kLabelWidth + static_cast<double>(x0), 0, static_cast<double>(x1 - x0 + 1), size.height()),
                             kBackground);
        }
        for (std::size_t r = 0; r < lane.rows; ++r) {
            const bool output = withOutput && r + 1 == lane.rows;
            m_points.clear();
            m_breaks.clear();
            // One column of overlap joins the new piece to the trace already in the image.
            const float rangeMax = collect(lane, r, x0 > 0 ? x0 - 1 : 0, x1);
            if (full && rangeMax > 0.0F && (lane.paintedSweep != sweep || lane.ranges[r] == 0.0F)) {
                lane.ranges[r] = niceRange(rangeMax); // sensitivity changes only when a sweep starts
            }
            const float range = output ? static_cast<float>(1.2e-3) : (lane.ranges[r] > 0.0F ? lane.ranges[r] : 1.0F);
            const double centre = (static_cast<double>(r) + 0.5) * rowHeight;
            const double gain = 0.45 * rowHeight / static_cast<double>(range);
            for (QPointF& p : m_points) {
                p.setY(centre - gain * p.y());
            }
            const ChannelStyle style = output ? ChannelStyle{kStimulus, "A"} : lane.styles[r];
            painter.setPen(style.colour);
            // A value beyond the sensitivity held for this sweep stays in its own row.
            painter.setClipRect(QRectF(kLabelWidth, centre - rowHeight / 2, lane.width, rowHeight));
            m_breaks.push_back(m_points.size());
            for (std::size_t b = 0; b + 1 < m_breaks.size(); ++b) {
                const std::size_t n = m_breaks[b + 1] - m_breaks[b];
                if (n > 1) {
                    painter.drawPolyline(m_points.data() + m_breaks[b], static_cast<int>(n));
                }
            }
            painter.setClipping(false);
            if (full) {
                // Name on the left, sensitivity right-aligned before the trace, one line per channel.
                const QRectF label(8, centre - rowHeight / 2, kLabelWidth - 16, rowHeight);
                painter.setFont(m_labelFont);
                painter.setPen(kLabel);
                painter.drawText(label, Qt::AlignVCenter | Qt::AlignLeft,
                                 QString::fromStdString(output ? std::string("STIM OUT") : lane.names[r]));
                painter.setFont(m_scaleFont);
                painter.setPen(kScale);
                painter.drawText(label, Qt::AlignVCenter | Qt::AlignRight, formatRange(static_cast<double>(range), style.unit));
            }
        }
        if (full) {
            painter.setPen(kSeparator);
            painter.drawLine(QPointF(4, 0), QPointF(width() - 4, 0));
            painter.setFont(m_scaleFont);
            painter.setPen(kTitle);
            painter.drawText(QRectF(kLabelWidth, 0, lane.width, 14), Qt::AlignRight | Qt::AlignTop,
                             QString::fromStdString(lane.title));
        }
        lane.painted = lane.newest;
        lane.paintedSweep = sweep;
    }

    /** Smallest 1-2-5 step at or above @p value. */
    static float niceRange(float value)
    {
        const double decade = std::pow(10.0, std::floor(std::log10(static_cast<double>(value))));
        for (const double step : {1.0, 2.0, 5.0, 10.0}) {
            if (step * decade >= static_cast<double>(value)) {
                return static_cast<float>(step * decade);
            }
        }
        return value;
    }

    Shared& m_shared;
    QFont m_labelFont;
    QFont m_scaleFont;
    std::vector<Lane> m_lanes;
    std::vector<Marker> m_triggers;
    std::vector<Marker> m_pulses;
    std::vector<QPointF> m_points;
    std::vector<std::size_t> m_breaks;
};

QString statusText(const Shared& s)
{
    return QString("views %1 (%2 across the end of the input ring), %3 samples lost  |  pulses %4 scheduled, "
                   "%5 written across the end of the output ring, %6 refused  |  played samples off the pulse: %7")
        .arg(s.views.load())
        .arg(s.wrappedViews.load())
        .arg(s.lost.load())
        .arg(s.pulsesScheduled.load())
        .arg(s.pulsesWrapped.load())
        .arg(s.pulsesRefused.load())
        .arg(s.playedMismatches.load());
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::string uri = "synthetic://?eeg=8&fnirs=2&ecog=8&erg=2&seed=3&interval=0.7";
    double seconds = 0.0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--seconds" && i + 1 < argc) {
            char* end = nullptr;
            seconds = std::strtod(argv[++i], &end);
            if (*end != '\0' || !(seconds > 0.0)) {
                std::fprintf(stderr, "--seconds needs a positive number, got %s\n", argv[i]);
                return 1;
            }
        } else {
            uri = arg;
        }
    }

    auto device = AcqDevice::open(uri);
    if (!device) {
        std::fprintf(stderr, "Cannot open %s: %s\n", uri.c_str(), device.error().text().c_str());
        return 1;
    }
    const auto info = device->stream().info();
    if (isComplex(info->format) || info->sfreq <= 0.0) {
        std::fprintf(stderr, "%s needs a regularly sampled real-valued stream\n", uri.c_str());
        return 1;
    }
    const double sfreq = info->sfreq;

    //! [acq_scope_output]
    // Stimulator output on the input's clock; a small ring makes pulses cross its end often.
    AcqStreamInfo outInfo;
    outInfo.name = "stim";
    outInfo.direction = AcqDirection::Output;
    outInfo.format = AcqSampleFormat::Int16;
    outInfo.sfreq = sfreq;
    outInfo.channels.resize(1);
    outInfo.channels[0].name = "STIM OUT";
    outInfo.channels[0].kind = AcqChannelKind::StimCurrent;
    outInfo.channels[0].unit = AcqUnit::Ampere;
    outInfo.channels[0].scale = kFullScale / 32767.0;
    outInfo.channels[0].rangeMin = -kFullScale;
    outInfo.channels[0].rangeMax = kFullScale;
    outInfo.underrunFill = AcqUnderrunFill::Zero;
    AcqStreamOptions outOptions;
    outOptions.capacitySamples = std::bit_ceil(static_cast<std::size_t>(
        std::llround((kMaxViewSeconds + kDelaySeconds + 2.0 * kPhaseSeconds) * sfreq) + 1));
    auto stimulator = AcqWriter::create(outInfo, outOptions);
    if (!stimulator) {
        std::fprintf(stderr, "Cannot create the stimulator: %s\n", stimulator.error().text().c_str());
        return 1;
    }
    AcqReaderOptions gating;
    gating.gating = true; // the emulated DAC: it consumes what was written or plays fill
    auto dac = stimulator->stream().reader(AcqReader::ReadMode::Lossless, gating);
    //! [acq_scope_output]
    auto input = device->stream().reader(AcqReader::ReadMode::Lossless);
    Shared shared;
    shared.traces.emplace_back(*info, 1);
    // Every further real-valued stream at its own rate: fNIRS, ECoG, ERG, ...
    std::vector<AcqReader> others;
    for (std::size_t i = 1; i < device->streams().size(); ++i) {
        const AcqStream stream = device->stream(i);
        const auto streamInfo = stream.info();
        if (streamInfo->direction != AcqDirection::Input || isComplex(streamInfo->format) || streamInfo->sfreq <= 0.0) {
            continue;
        }
        if (auto reader = stream.reader(AcqReader::ReadMode::Lossless)) {
            others.push_back(std::move(*reader));
            shared.traces.emplace_back(*streamInfo, 0);
        }
    }
    if (!dac || !input || !device->start().ok()) {
        std::fprintf(stderr, "Cannot start %s\n", uri.c_str());
        return 1;
    }

    std::thread worker(acquire, std::ref(*input), std::ref(others), std::ref(*stimulator), std::ref(*dac),
                       std::ref(shared));

    QWidget window;
    QString title = QString::fromStdString(device->description().device);
    for (const Trace& trace : shared.traces) {
        title += QString("  |  %1").arg(QString::fromStdString(trace.title));
    }
    window.setWindowTitle(title);
    window.setStyleSheet(QString("background-color: %1;").arg(kBackground.name()));
    auto* layout = new QVBoxLayout(&window);
    layout->setContentsMargins(8, 8, 8, 6);
    auto* scope = new ScopeView(shared);
    auto* status = new QLabel;
    status->setWordWrap(true);
    status->setStyleSheet(QString("color: %1;").arg(kScale.name()));
    layout->addWidget(scope, 1);
    layout->addWidget(status);
    window.resize(1400, 900);

    QTimer refresh;
    QObject::connect(&refresh, &QTimer::timeout, [&] {
        status->setText(statusText(shared));
        scope->update();
    });
    refresh.start(33);
    if (seconds > 0.0) {
        QTimer::singleShot(static_cast<int>(seconds * 1000.0), &app, [&] {
            status->setText(statusText(shared));
            FigureExporter::save(window, "acq_scope",
                                 QString("Live sweep of %1 streams at their own rates on one clock: %2. The "
                                         "stimulator output is played back below the first stream; %3 of %4 "
                                         "pulses crossed the end of the output ring.")
                                     .arg(shared.traces.size())
                                     .arg(title)
                                     .arg(shared.pulsesWrapped.load())
                                     .arg(shared.pulsesScheduled.load())
                                     .toStdString(),
                                 window.size());
            app.quit();
        });
    }
    window.show();
    app.exec();

    shared.stop = true;
    worker.join();
    device->close();
    std::printf("%s\n", statusText(shared).toStdString().c_str());
    const bool ok = shared.playedMismatches == 0 && shared.lost == 0 && shared.pulsesRefused == 0;
    std::printf("%s\n", ok ? "PASS: every played sample on its scheduled pulse" : "FAIL: see the counters above");
    return ok ? 0 : 1;
}
