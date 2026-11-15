//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_synthetic_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    synthetic:// : a seeded auditory-oddball EEG/MEG session.
 *
 * Every sample is a pure function of the seed, the sample index and the channel, so a recording does not
 * depend on block size, pacing or restarts. Signal per channel c (1 model unit = 1 uV on EEG, 20 fT on MEG):
 *
 *     x_c(s) = noise * 10 * N(0, 1) + 8 * sin(2 pi alpha t + d(t) + phi_c) + evoked * w_c * sum_k erp_k(s)
 *
 * with t = s / sfreq and d(t) a smooth phase drift (cosine-interpolated random values every 0.5 s, within
 * +-1.5 pi), so the rhythm is not phase-locked to the stimuli, as in real recordings. w_c = cos(pi c / n) (a dipolar topography over the n channels of one kind). Trial k starts at
 * (k + 1 + 0.1 u_k) * interval seconds; it is a target with probability targets, a standard otherwise.
 * Standards evoke an N100 (-5 at 100 ms, sigma 25 ms) and a P200 (+4 at 200 ms, sigma 40 ms); targets add
 * a P300 (+8 at 350 ms, sigma 80 ms). The stim channel holds the trial code (1 standard, 2 target) for 5 ms.
 *
 * Further modalities are separate streams at their own rates, like the separate amplifiers of a multimodal
 * session, all on one clock domain so their samples map onto each other exactly (rates must then be whole Hz;
 * the domain tick is their least common multiple). Each stream carries the trial events at the first of its
 * samples at or after the onset. Per stimulus (flash and tone of a visual-auditory oddball):
 * - fnirs: source-detector pairs with HbO and HbR (M): canonical haemodynamic response (peak 5 s, undershoot
 *   15 s, HbR -0.3 x HbO), Mayer waves (0.1 Hz), respiration (0.25 Hz), cardiac pulsation (1 Hz).
 * - ecog (V): the ERP at 4x the EEG amplitude plus a high-gamma burst (80-140 Hz, 150 ms after onset) on a focal
 *   group of contacts.
 * - erg (V): the flash ERG: a-wave (-80 uV at 15 ms), b-wave (+160 uV at 40 ms), oscillatory potentials
 *   (120 Hz around 30 ms).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../detail/acq_synthetic_backend.h"

#include "../detail/acq_uri_params.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <numeric>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{

using Clock = std::chrono::steady_clock;

constexpr double kPi = 3.14159265358979323846;
constexpr double kEegUnit = 1e-6;   // V per model unit
constexpr double kMegUnit = 20e-15; // T per model unit
constexpr double kNoiseRms = 10.0;
constexpr double kAlphaAmplitude = 8.0;
constexpr double kDriftKnotSeconds = 0.5;
constexpr double kDriftRange = 1.5 * 3.14159265358979323846;
constexpr double kWindowSeconds = 0.8; // evoked responses have decayed below 1e-4 of their peak by then
constexpr double kJitter = 0.1;
constexpr double kPulseSeconds = 0.005;
constexpr std::int32_t kStandard = 1;
constexpr std::int32_t kTarget = 2;

struct Component
{
    double amplitude;
    double latency;
    double width;

    [[nodiscard]] double at(double t) const noexcept
    {
        const double z = (t - latency) / width;
        return amplitude * std::exp(-0.5 * z * z);
    }
};

constexpr Component kN100{-5.0, 0.100, 0.025};
constexpr Component kP200{4.0, 0.200, 0.040};
constexpr Component kP300{8.0, 0.350, 0.080};

enum Stream : std::uint64_t
{
    NoiseStream = 1,
    PhaseStream = 2,
    TrialStream = 3,
    JitterStream = 4,
    DriftStream = 5,
    FnirsNoiseStream = 6,
    EcogNoiseStream = 7,
    ErgNoiseStream = 8,
    ModalityPhaseStream = 9,
    ImpedanceStream = 10
};

constexpr double kFnirsUnit = 1e-6; // M per model unit (1 uM)
constexpr double kFnirsWindowSeconds = 32.0;
constexpr double kEcogWindowSeconds = 0.8;
constexpr double kErgWindowSeconds = 0.25;
constexpr double kHighGammaHz[3] = {80.0, 110.0, 140.0};
constexpr double kOscillatoryPotentialHz = 120.0;

constexpr Component kAWave{-80.0, 0.015, 0.005};
constexpr Component kBWave{160.0, 0.040, 0.012};
constexpr Component kHighGamma{6.0, 0.150, 0.050};

constexpr std::uint64_t mix(std::uint64_t x) noexcept
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31U);
}

constexpr std::uint64_t hash(std::uint64_t seed, Stream stream, std::uint64_t a, std::uint64_t b = 0) noexcept
{
    return mix(mix(mix(seed ^ (stream * 0xD6E8FEB86659FD93ULL)) ^ a) ^ b);
}

double uniform(std::uint64_t h) noexcept
{
    return static_cast<double>(h >> 11U) * 0x1.0p-53;
}

/** Two independent standard normal values from one hash (Box-Muller). */
std::array<double, 2> gaussianPair(std::uint64_t h) noexcept
{
    const double u1 = (static_cast<double>(h >> 32U) + 0.5) * 0x1.0p-32;
    const double u2 = (static_cast<double>(h & 0xFFFFFFFFULL) + 0.5) * 0x1.0p-32;
    const double r = std::sqrt(-2.0 * std::log(u1));
    return {r * std::cos(2.0 * kPi * u2), r * std::sin(2.0 * kPi * u2)};
}

/** Canonical double-gamma haemodynamic response (SPM), 1 at its peak. */
class Hrf
{
public:
    Hrf()
    {
        for (int i = 1; i < 3200; ++i) {
            m_peak = std::max(m_peak, raw(0.01 * i));
        }
    }

    [[nodiscard]] double at(double t) const noexcept
    {
        return t <= 0.0 ? 0.0 : raw(t) / m_peak;
    }

private:
    static double raw(double t) noexcept
    {
        constexpr double kGamma6 = 120.0;
        constexpr double kGamma16 = 1307674368000.0;
        return std::pow(t, 5.0) * std::exp(-t) / kGamma6 - std::pow(t, 15.0) * std::exp(-t) / (6.0 * kGamma16);
    }

    double m_peak = 0.0;
};

enum class Modality : std::uint8_t
{
    Fnirs,
    Ecog,
    Erg
};

//=============================================================================================================

struct Config
{
    std::size_t eeg = 8;
    std::size_t meg = 0;
    bool stim = true;
    double sfreq = 1000.0;
    std::size_t block = 0;
    bool realtime = true;
    double speed = 1.0;
    std::uint64_t totalSamples = 0;
    std::uint64_t seed = 0;
    double noise = 1.0;
    double alpha = 10.0;
    double interval = 1.0;
    double targets = 0.2;
    double evoked = 1.0;
    std::uint64_t pulse = 1;
    std::size_t fnirs = 0; // source-detector pairs, two channels (HbO, HbR) each
    std::size_t ecog = 0;
    std::size_t erg = 0;
    double fnirsSfreq = 10.0;
    double ecogSfreq = 2000.0;
    double ergSfreq = 2000.0;
    std::uint64_t ticksPerSecond = 0; // shared clock domain; 0 = the EEG/MEG rate is not a whole number of Hz
    AcqThreadPolicy thread;           // placement and priority of the producer thread
};

bool wholeHz(double rate) noexcept
{
    return rate >= 1.0 && rate == std::round(rate);
}

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    if (!uri.target.empty()) {
        return invalid("target '" + uri.target + "': synthetic:// takes no target, only parameters");
    }
    const auto eeg = paramInt(uri, "eeg", 8, 0, 4096);
    const auto meg = paramInt(uri, "meg", 0, 0, 4096);
    const auto stim = paramBool(uri, "stim", true);
    const auto sfreq = paramDouble(uri, "sfreq", 1000.0, 0.001, 1e6);
    const auto block = paramInt(uri, "block", 0, 1, 1 << 20);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto speed = paramDouble(uri, "speed", 1.0, 0.01, 1e4);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto noise = paramDouble(uri, "noise", 1.0, 0.0, 1e3);
    const auto alpha = paramDouble(uri, "alpha", 10.0, 0.0, 5e5);
    const auto interval = paramDouble(uri, "interval", 1.0, 0.0, 3600.0);
    const auto targets = paramDouble(uri, "targets", 0.2, 0.0, 1.0);
    const auto evoked = paramDouble(uri, "evoked", 1.0, 0.0, 1e3);
    const auto fnirs = paramInt(uri, "fnirs", 0, 0, 1024);
    const auto ecog = paramInt(uri, "ecog", 0, 0, 4096);
    const auto erg = paramInt(uri, "erg", 0, 0, 64);
    const auto fnirsSfreq = paramDouble(uri, "fnirs_sfreq", 10.0, 3.0, 1e3);
    const auto ecogSfreq = paramDouble(uri, "ecog_sfreq", 2000.0, 300.0, 1e6);
    const auto ergSfreq = paramDouble(uri, "erg_sfreq", 2000.0, 300.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error :
         {eeg.error(), meg.error(), stim.error(), sfreq.error(), block.error(), realtime.error(), speed.error(),
          duration.error(), seed.error(), noise.error(), alpha.error(), interval.error(), targets.error(),
          evoked.error(), fnirs.error(), ecog.error(), erg.error(), fnirsSfreq.error(), ecogSfreq.error(),
          ergSfreq.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.thread = std::move(*thread);
    c.fnirs = static_cast<std::size_t>(*fnirs);
    c.ecog = static_cast<std::size_t>(*ecog);
    c.erg = static_cast<std::size_t>(*erg);
    c.fnirsSfreq = *fnirsSfreq;
    c.ecogSfreq = *ecogSfreq;
    c.ergSfreq = *ergSfreq;
    c.eeg = static_cast<std::size_t>(*eeg);
    c.meg = static_cast<std::size_t>(*meg);
    c.stim = *stim;
    c.sfreq = *sfreq;
    c.block = *block > 0 ? static_cast<std::size_t>(*block) : static_cast<std::size_t>(std::max(1.0, std::round(c.sfreq / 100.0)));
    c.realtime = *realtime;
    c.speed = *speed;
    c.totalSamples = static_cast<std::uint64_t>(std::llround(*duration * c.sfreq));
    c.seed = static_cast<std::uint64_t>(*seed);
    c.noise = *noise;
    c.alpha = *alpha;
    c.interval = *interval;
    c.targets = *targets;
    c.evoked = *evoked;
    c.pulse = static_cast<std::uint64_t>(std::max(1.0, std::ceil(kPulseSeconds * c.sfreq - 1e-9)));

    if (c.eeg + c.meg == 0 && !c.stim) {
        return invalid("eeg=0, meg=0 and stim=0 leave synthetic:// without channels");
    }
    if (c.alpha >= c.sfreq / 2.0) {
        return invalid("alpha=" + std::to_string(c.alpha) + " must stay below the Nyquist frequency sfreq/2");
    }
    if (c.interval > 0.0 && c.interval * c.sfreq < 2.0 * static_cast<double>(c.pulse)) {
        return invalid("interval=" + std::to_string(c.interval) + " is shorter than two 5 ms trigger pulses");
    }
    // One clock domain: every rate a whole number of Hz, the domain tick their least common multiple.
    const std::pair<const char*, double> rates[] = {{"sfreq", c.sfreq},
                                                    {"fnirs_sfreq", c.fnirs > 0 ? c.fnirsSfreq : 0.0},
                                                    {"ecog_sfreq", c.ecog > 0 ? c.ecogSfreq : 0.0},
                                                    {"erg_sfreq", c.erg > 0 ? c.ergSfreq : 0.0}};
    const bool extra = c.fnirs + c.ecog + c.erg > 0;
    std::uint64_t ticks = 1;
    for (const auto& [name, rate] : rates) {
        if (rate == 0.0) {
            continue;
        }
        if (!wholeHz(rate)) {
            if (extra) {
                return invalid(std::string(name) + "=" + std::to_string(rate) +
                               " must be a whole number of Hz for the streams to share the device clock");
            }
            return c; // a single stream at a fractional rate: no exact clock domain
        }
        ticks = std::lcm(ticks, static_cast<std::uint64_t>(rate));
    }
    c.ticksPerSecond = ticks;
    return c;
}

//=============================================================================================================

struct Trial
{
    std::uint64_t onset = 0;
    std::int32_t code = 0;
};

/** The signal model; stateless after construction, so any sample can be computed in any order. */
class Model
{
public:
    explicit Model(const Config& config)
    : m_c(config)
    , m_analog(config.eeg + config.meg)
    , m_window(static_cast<std::uint64_t>(std::ceil(kWindowSeconds * config.sfreq)))
    {
        for (std::size_t c = 0; c < m_analog; ++c) {
            const bool eeg = c < m_c.eeg;
            const std::size_t index = eeg ? c : c - m_c.eeg;
            const std::size_t count = eeg ? m_c.eeg : m_c.meg;
            m_weight.push_back(std::cos(kPi * static_cast<double>(index) / static_cast<double>(count)));
            m_phase.push_back(2.0 * kPi * uniform(hash(m_c.seed, PhaseStream, c)));
            m_unit.push_back(eeg ? kEegUnit : kMegUnit);
        }
    }

    [[nodiscard]] std::size_t channels() const noexcept
    {
        return m_analog + (m_c.stim ? 1U : 0U);
    }

    [[nodiscard]] Trial trial(std::uint64_t k) const noexcept
    {
        const double start = static_cast<double>(k + 1) + kJitter * uniform(hash(m_c.seed, JitterStream, k));
        Trial t;
        t.onset = static_cast<std::uint64_t>(std::llround(start * m_c.interval * m_c.sfreq));
        t.code = uniform(hash(m_c.seed, TrialStream, k)) < m_c.targets ? kTarget : kStandard;
        return t;
    }

    /** @return Upper bound of the trials whose responses of @p window samples touch a block of n samples. */
    [[nodiscard]] std::size_t maxTrials(std::size_t n, std::uint64_t window) const noexcept
    {
        if (m_c.interval <= 0.0) {
            return 0;
        }
        return static_cast<std::size_t>(static_cast<double>(n + window) / (m_c.interval * m_c.sfreq)) + 4;
    }

    [[nodiscard]] std::size_t maxTrials(std::size_t n) const noexcept
    {
        return maxTrials(n, m_window);
    }

    /**
     * Collects the trials whose response of @p window samples or trigger overlaps samples [first, last].
     * Allocation-free within maxTrials.
     */
    void trials(std::uint64_t first, std::uint64_t last, std::uint64_t window, std::vector<Trial>& out) const
    {
        out.clear();
        if (m_c.interval <= 0.0) {
            return;
        }
        const double perTrial = m_c.interval * m_c.sfreq;
        const std::uint64_t low = first > window ? first - window : 0;
        const double kLow = std::floor(static_cast<double>(low) / perTrial) - 2.0;
        const auto kMin = static_cast<std::uint64_t>(std::max(0.0, kLow));
        const auto kMax = static_cast<std::uint64_t>(static_cast<double>(last) / perTrial);
        for (std::uint64_t k = kMin; k <= kMax; ++k) {
            const Trial t = trial(k);
            if (t.onset <= last && t.onset + window > first) {
                out.push_back(t);
            }
        }
    }

    void trials(std::uint64_t first, std::uint64_t last, std::vector<Trial>& out) const
    {
        trials(first, last, m_window, out);
    }

    /** @return The phase drift of the rhythm at t seconds. */
    [[nodiscard]] double drift(double t) const noexcept
    {
        const double knot = t / kDriftKnotSeconds;
        const double k = std::floor(knot);
        const double w = 0.5 - 0.5 * std::cos(kPi * (knot - k));
        const auto index = static_cast<std::uint64_t>(k);
        const double a = 2.0 * uniform(hash(m_c.seed, DriftStream, index)) - 1.0;
        const double b = 2.0 * uniform(hash(m_c.seed, DriftStream, index + 1)) - 1.0;
        return kDriftRange * (a + (b - a) * w);
    }

    /** Writes the frame of sample s (all channels, physical units) to out. */
    void frame(std::uint64_t s, const std::vector<Trial>& trials, float* out) const noexcept
    {
        double erp = 0.0;
        float trigger = 0.0F;
        for (const Trial& t : trials) {
            if (s < t.onset || s - t.onset >= m_window) {
                continue;
            }
            const double latency = static_cast<double>(s - t.onset) / m_c.sfreq;
            erp += kN100.at(latency) + kP200.at(latency) + (t.code == kTarget ? kP300.at(latency) : 0.0);
            if (s - t.onset < m_c.pulse) {
                trigger = static_cast<float>(t.code);
            }
        }
        erp *= m_c.evoked;

        const double cycles = m_c.alpha * static_cast<double>(s) / m_c.sfreq;
        const double alphaPhase = 2.0 * kPi * (cycles - std::floor(cycles)) + drift(static_cast<double>(s) / m_c.sfreq);
        for (std::size_t c = 0; c < m_analog; c += 2) {
            double z[2] = {0.0, 0.0};
            if (m_c.noise > 0.0) {
                const std::uint64_t h = hash(m_c.seed, NoiseStream, s, c / 2);
                const double u1 = (static_cast<double>(h >> 32U) + 0.5) * 0x1.0p-32;
                const double u2 = (static_cast<double>(h & 0xFFFFFFFFULL) + 0.5) * 0x1.0p-32;
                const double r = std::sqrt(-2.0 * std::log(u1));
                z[0] = r * std::cos(2.0 * kPi * u2);
                z[1] = r * std::sin(2.0 * kPi * u2);
            }
            for (std::size_t j = 0; j < 2 && c + j < m_analog; ++j) {
                const std::size_t ch = c + j;
                double v = m_c.noise * kNoiseRms * z[j] + m_weight[ch] * erp;
                if (m_c.alpha > 0.0) {
                    v += kAlphaAmplitude * std::sin(alphaPhase + m_phase[ch]);
                }
                out[ch] = static_cast<float>(v * m_unit[ch]);
            }
        }
        if (m_c.stim) {
            out[m_analog] = trigger;
        }
    }

private:
    Config m_c;
    std::size_t m_analog;
    std::uint64_t m_window;
    std::vector<double> m_weight;
    std::vector<double> m_phase;
    std::vector<double> m_unit;
};

//=============================================================================================================

/**
 * A further modality at its own rate, sampled on the clock domain of the EEG/MEG stream: stream sample s lies at
 * domain tick s * ticksPerSample, EEG/MEG sample k (and so a trial onset) at k * baseTicksPerSample.
 */
class ModalityModel
{
public:
    ModalityModel(const Config& config, Modality modality)
    : m_c(config)
    , m_modality(modality)
    , m_sfreq(modality == Modality::Fnirs ? config.fnirsSfreq
                                          : (modality == Modality::Ecog ? config.ecogSfreq : config.ergSfreq))
    , m_ticksPerSample(config.ticksPerSecond / static_cast<std::uint64_t>(m_sfreq))
    , m_baseTicksPerSample(config.ticksPerSecond / static_cast<std::uint64_t>(config.sfreq))
    , m_count(modality == Modality::Fnirs ? config.fnirs : (modality == Modality::Ecog ? config.ecog : config.erg))
    {
        const double window = modality == Modality::Fnirs ? kFnirsWindowSeconds
                                                          : (modality == Modality::Ecog ? kEcogWindowSeconds
                                                                                        : kErgWindowSeconds);
        m_window = static_cast<std::uint64_t>(std::ceil(window * config.sfreq)) + 1;
        m_pulse = static_cast<std::uint64_t>(std::max(1.0, std::ceil(kPulseSeconds * m_sfreq - 1e-9)));
        const auto base = static_cast<std::uint64_t>(modality) * 100000;
        for (std::size_t c = 0; c < 4 * m_count + 3; ++c) {
            m_phase.push_back(2.0 * kPi * uniform(hash(m_c.seed, ModalityPhaseStream, base + c)));
        }
        for (std::size_t c = 0; c < m_count; ++c) {
            const double x = static_cast<double>(c) / static_cast<double>(m_count);
            m_weight.push_back(modality == Modality::Erg ? 0.9 + 0.2 * uniform(hash(m_c.seed, ModalityPhaseStream, base + 50000 + c))
                                                         : std::cos(kPi * x));
            const double focus = (x - 0.3) / (0.1 + 1.0 / static_cast<double>(m_count));
            m_focal.push_back(std::exp(-0.5 * focus * focus));
        }
    }

    [[nodiscard]] Modality modality() const noexcept
    {
        return m_modality;
    }

    [[nodiscard]] double sfreq() const noexcept
    {
        return m_sfreq;
    }

    [[nodiscard]] std::uint64_t ticksPerSample() const noexcept
    {
        return m_ticksPerSample;
    }

    [[nodiscard]] std::size_t channels() const noexcept
    {
        return m_modality == Modality::Fnirs ? 2 * m_count : m_count;
    }

    /** @return Response length in EEG/MEG samples, for Model::trials(). */
    [[nodiscard]] std::uint64_t window() const noexcept
    {
        return m_window;
    }

    [[nodiscard]] std::uint64_t pulse() const noexcept
    {
        return m_pulse;
    }

    /** @return The first sample of this stream at or after EEG/MEG sample @p onset. */
    [[nodiscard]] std::uint64_t sampleAtOrAfter(std::uint64_t onset) const noexcept
    {
        return (onset * m_baseTicksPerSample + m_ticksPerSample - 1) / m_ticksPerSample;
    }

    /** @return The EEG/MEG samples [first, last] spanned by samples [from, to] of this stream. */
    [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> baseRange(std::uint64_t from, std::uint64_t to) const noexcept
    {
        return {from * m_ticksPerSample / m_baseTicksPerSample,
                (to * m_ticksPerSample + m_baseTicksPerSample - 1) / m_baseTicksPerSample};
    }

    /** Writes the frame of sample @p s (all channels, physical units) to @p out. */
    void frame(std::uint64_t s, const std::vector<Trial>& trials, float* out) const noexcept
    {
        const auto tick = static_cast<std::int64_t>(s * m_ticksPerSample);
        const double t = static_cast<double>(tick) / static_cast<double>(m_c.ticksPerSecond);
        double slow = 0.0; // ERP (ECoG), haemodynamic response (fNIRS), flash ERG
        double fast = 0.0; // high-gamma envelope (ECoG)
        for (const Trial& trial : trials) {
            const std::int64_t delta = tick - static_cast<std::int64_t>(trial.onset * m_baseTicksPerSample);
            const double latency = static_cast<double>(delta) / static_cast<double>(m_c.ticksPerSecond);
            if (latency < 0.0) {
                continue;
            }
            const double gain = trial.code == kTarget ? 1.5 : 1.0;
            switch (m_modality) {
                case Modality::Fnirs:
                    slow += latency < kFnirsWindowSeconds ? gain * m_hrf.at(latency) : 0.0;
                    break;
                case Modality::Ecog:
                    if (latency < kEcogWindowSeconds) {
                        slow += kN100.at(latency) + kP200.at(latency) + (trial.code == kTarget ? kP300.at(latency) : 0.0);
                        fast += gain * kHighGamma.at(latency);
                    }
                    break;
                case Modality::Erg:
                    if (latency < kErgWindowSeconds) {
                        const double op = (latency - 0.030) / 0.008;
                        slow += kAWave.at(latency) + kBWave.at(latency) +
                            12.0 * std::sin(2.0 * kPi * kOscillatoryPotentialHz * latency) * std::exp(-0.5 * op * op);
                    }
                    break;
            }
        }
        slow *= m_c.evoked;
        fast *= m_c.evoked;

        if (m_modality == Modality::Fnirs) {
            const std::size_t g = 4 * m_count;
            const double mayer = m_c.noise * 0.2 * std::sin(2.0 * kPi * 0.1 * t + m_phase[g]);
            const double breath = m_c.noise * 0.1 * std::sin(2.0 * kPi * 0.25 * t + m_phase[g + 1]);
            const double pulse = m_c.noise * 0.15 * std::sin(2.0 * kPi * 1.0 * t + m_phase[g + 2]);
            for (std::size_t i = 0; i < m_count; ++i) {
                const auto z = gaussianPair(hash(m_c.seed, FnirsNoiseStream, s, i));
                const double activation = 0.4 * m_weight[i] * slow;
                const double hbo = activation + mayer + breath + pulse + m_c.noise * 0.03 * z[0];
                const double hbr = -0.3 * activation - 0.3 * mayer - 0.1 * breath + 0.1 * pulse + m_c.noise * 0.03 * z[1];
                out[2 * i] = static_cast<float>(hbo * kFnirsUnit);
                out[2 * i + 1] = static_cast<float>(hbr * kFnirsUnit);
            }
            return;
        }
        const Stream noiseStream = m_modality == Modality::Ecog ? EcogNoiseStream : ErgNoiseStream;
        const double noiseRms = m_modality == Modality::Ecog ? 25.0 : 5.0;
        for (std::size_t c = 0; c < m_count; c += 2) {
            const auto z = m_c.noise > 0.0 ? gaussianPair(hash(m_c.seed, noiseStream, s, c / 2)) : std::array<double, 2>{};
            for (std::size_t j = 0; j < 2 && c + j < m_count; ++j) {
                const std::size_t ch = c + j;
                double v = m_c.noise * noiseRms * z[j];
                if (m_modality == Modality::Ecog) {
                    double burst = 0.0;
                    for (std::size_t f = 0; f < 3; ++f) {
                        burst += std::sin(2.0 * kPi * kHighGammaHz[f] * t + m_phase[3 * ch + f]);
                    }
                    v += 4.0 * m_weight[ch] * slow + m_focal[ch] * fast * burst / 3.0;
                } else {
                    v += m_weight[ch] * slow;
                }
                out[ch] = static_cast<float>(v * kEegUnit);
            }
        }
    }

private:
    Config m_c;
    Modality m_modality;
    double m_sfreq;
    std::uint64_t m_ticksPerSample;
    std::uint64_t m_baseTicksPerSample;
    std::size_t m_count;
    std::uint64_t m_window = 0;
    std::uint64_t m_pulse = 1;
    std::vector<double> m_phase;
    std::vector<double> m_weight;
    std::vector<double> m_focal;
    Hrf m_hrf;
};

//=============================================================================================================

/** Channel name with a zero-padded 1-based index, e.g. "EEG 001". */
std::string numbered(const char* prefix, std::size_t i)
{
    std::string digits = std::to_string(i + 1);
    return std::string(prefix) + std::string(digits.size() < 3 ? 3 - digits.size() : 0, '0') + digits;
}

AcqStreamInfo modalityInfo(const Config& c, const ModalityModel& m)
{
    AcqStreamInfo info;
    info.name = m.modality() == Modality::Fnirs ? "fnirs" : (m.modality() == Modality::Ecog ? "ecog" : "erg");
    info.sourceId = "synthetic:seed=" + std::to_string(c.seed);
    info.format = AcqSampleFormat::Float32;
    info.sfreq = m.sfreq();
    info.eventLabels = {"", "standard", "target"};
    for (std::size_t i = 0; i < m.channels(); ++i) {
        AcqChannelInfo channel;
        switch (m.modality()) {
            case Modality::Fnirs:
                channel.name = "S" + std::to_string(i / 2 + 1) + "_D" + std::to_string(i / 2 + 1) + (i % 2 == 0 ? " hbo" : " hbr");
                channel.kind = AcqChannelKind::Fnirs;
                channel.unit = AcqUnit::Molar;
                break;
            case Modality::Ecog:
                channel.name = numbered("ECOG ", i);
                channel.kind = AcqChannelKind::Ecog;
                channel.unit = AcqUnit::Volt;
                break;
            case Modality::Erg:
                channel.name = numbered("ERG ", i);
                channel.kind = AcqChannelKind::Erg;
                channel.unit = AcqUnit::Volt;
                break;
        }
        info.channels.push_back(std::move(channel));
    }
    return info;
}

AcqStreamInfo streamInfo(const Config& c)
{
    AcqStreamInfo info;
    info.name = "synthetic";
    info.sourceId = "synthetic:seed=" + std::to_string(c.seed);
    info.format = AcqSampleFormat::Float32;
    info.sfreq = c.sfreq;
    info.eventLabels = {"", "standard", "target"};
    auto add = [&info](std::string name, AcqChannelKind kind, AcqUnit unit) {
        AcqChannelInfo channel;
        channel.name = std::move(name);
        channel.kind = kind;
        channel.unit = unit;
        info.channels.push_back(std::move(channel));
    };
    for (std::size_t i = 0; i < c.eeg; ++i) {
        add(numbered("EEG ", i), AcqChannelKind::Eeg, AcqUnit::Volt);
    }
    for (std::size_t i = 0; i < c.meg; ++i) {
        add(numbered("MEG ", i), AcqChannelKind::Meg, AcqUnit::Tesla);
    }
    if (c.stim) {
        add("STI 014", AcqChannelKind::Stim, AcqUnit::None);
    }
    return info;
}

//=============================================================================================================

/** A further modality's stream with its producer state. */
struct ModalityStream
{
    ModalityModel model;
    AcqWriter* writer = nullptr;
    std::uint64_t next = 0;  // next sample index; owned by the producer thread while it runs
    std::uint64_t total = 0; // samples in a session of fixed duration, 0 = unbounded
    std::size_t pending = 0; // samples written in the current block, committed after the pacing wait
    std::vector<Trial> trials;
    std::vector<float> frame;
};

class SyntheticBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<SyntheticBackend> backend(new SyntheticBackend(*config, uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = 4 * config->block > 65536 ? 4 * config->block : 0;
        auto writer = backend->addInputStream(streamInfo(*config), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        for (const auto& [modality, count] : {std::pair{Modality::Fnirs, config->fnirs}, std::pair{Modality::Ecog, config->ecog},
                                              std::pair{Modality::Erg, config->erg}}) {
            if (count == 0) {
                continue;
            }
            ModalityStream stream{ModalityModel(*config, modality), nullptr, 0, 0, 0, {}, {}};
            auto added = backend->addInputStream(modalityInfo(*config, stream.model));
            if (!added) {
                return added.error();
            }
            stream.writer = *added;
            stream.trials.reserve(backend->m_model.maxTrials(config->block + 2, stream.model.window()));
            stream.frame.resize(stream.model.channels());
            stream.total = config->totalSamples == 0
                ? 0
                : stream.model.sampleAtOrAfter(config->totalSamples);
            backend->m_streams.push_back(std::move(stream));
        }
        backend->publishTimebases();
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~SyntheticBackend() override
    {
        stop();
    }

    SyntheticBackend(const SyntheticBackend&) = delete;
    SyntheticBackend& operator=(const SyntheticBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        std::string text = "Synthetic oddball session, seed " + std::to_string(m_config.seed);
        for (const ModalityStream& stream : m_streams) {
            text += ", " + std::string(stream.model.modality() == Modality::Fnirs ? "fNIRS" : (stream.model.modality() == Modality::Ecog ? "ECoG" : "ERG")) +
                " at " + std::to_string(static_cast<long long>(stream.model.sfreq())) + " Hz";
        }
        return {"synthetic", "", text, m_uri};
    }

    AcqError start() override
    {
        m_stop.store(false, std::memory_order_relaxed);
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_config.sfreq;
        clock.anchorSample = m_next;
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() -
            now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);
        for (ModalityStream& stream : m_streams) {
            clock.sfreq = stream.model.sfreq();
            clock.anchorSample = stream.next;
            stream.writer->setClock(clock);
        }
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            return thread.error();
        }
        m_thread = std::move(*thread);
        return {};
    }

    /** Electrodes get seeded impedances in a typical range of their kind: wet EEG 2-20 kOhm, ECoG 1-5 kOhm, ERG 5-15 kOhm. */
    AcqExpected<std::vector<AcqImpedance>> measureImpedances(std::chrono::milliseconds /*timeout*/) override
    {
        std::vector<AcqImpedance> out;
        const std::span<const AcqStream> all = streams();
        for (std::size_t s = 0; s < all.size(); ++s) {
            const auto info = all[s].info();
            for (std::size_t c = 0; c < info->channels.size(); ++c) {
                const AcqChannelKind kind = info->channels[c].kind;
                const double u = uniform(hash(m_config.seed, ImpedanceStream, s, c));
                if (kind == AcqChannelKind::Eeg) {
                    out.push_back({s, c, 2e3 + 18e3 * u});
                } else if (kind == AcqChannelKind::Ecog) {
                    out.push_back({s, c, 1e3 + 4e3 * u});
                } else if (kind == AcqChannelKind::Erg) {
                    out.push_back({s, c, 5e3 + 10e3 * u});
                }
            }
        }
        return out;
    }

    void stop() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex); // pairs with the wait in run(), so no wake-up is lost
            m_stop.store(true, std::memory_order_release);
        }
        m_wake.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    SyntheticBackend(const Config& config, std::string uri)
    : m_config(config)
    , m_model(config)
    , m_uri(std::move(uri))
    {
    }

    /** All streams are one clock domain, private to this device; sample 0 of every stream starts at tick 0. */
    void publishTimebases()
    {
        if (m_config.ticksPerSecond == 0) {
            return;
        }
        static std::atomic<std::uint64_t> devices{0};
        AcqTimebase timebase;
        timebase.domain = acqClockDomainId("synthetic:" + std::to_string(devices.fetch_add(1)) + ":" + m_uri);
        timebase.ticksPerSecond = m_config.ticksPerSecond;
        timebase.ticksPerSample = m_config.ticksPerSecond / static_cast<std::uint64_t>(m_config.sfreq);
        timebase.lock = AcqLockState::PhaseLocked;
        m_writer->stream().setTimebase(timebase);
        for (ModalityStream& stream : m_streams) {
            timebase.ticksPerSample = stream.model.ticksPerSample();
            stream.writer->stream().setTimebase(timebase);
        }
    }

    /** Sets the samples of @p stream up to the instant of EEG/MEG sample @p end (exclusive) as pending. */
    static void plan(ModalityStream& stream, std::uint64_t end)
    {
        std::uint64_t until = stream.model.sampleAtOrAfter(end);
        if (stream.total > 0) {
            until = std::min(until, stream.total);
        }
        stream.pending = until > stream.next ? static_cast<std::size_t>(until - stream.next) : 0;
    }

    /**
     * Waits until every stream can take its next block, so a gating reader (backpressure) on one stream
     * never blocks while another stream's block is reserved; returns false on stop() or an error.
     */
    bool waitForSpace(std::size_t n)
    {
        constexpr std::chrono::milliseconds kPoll(20);
        while (!m_stop.load(std::memory_order_acquire)) {
            AcqError e = m_writer->waitForSpace(n, kPoll);
            for (ModalityStream& stream : m_streams) {
                if (e.ok()) {
                    e = stream.writer->waitForSpace(stream.pending, kPoll);
                }
            }
            if (e.ok()) {
                return true;
            }
            if (e.code() != AcqErrorCode::Timeout) {
                reportError(e);
                return false;
            }
        }
        return false;
    }

    /** Writes the pending samples of @p stream; commit() follows. */
    bool generate(ModalityStream& stream)
    {
        if (stream.pending == 0) {
            return true;
        }
        const std::uint64_t until = stream.next + stream.pending;
        auto view = stream.writer->beginWrite(stream.pending);
        if (!view) {
            reportError(view.error());
            return false;
        }
        const auto [first, last] = stream.model.baseRange(stream.next, until - 1);
        m_model.trials(first, last, stream.model.window(), stream.trials);
        std::uint64_t s = stream.next;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                stream.model.frame(s, stream.trials, stream.frame.data());
                std::memcpy(part.at(0, i), stream.frame.data(), stream.frame.size() * sizeof(float));
            }
        }
        return true;
    }

    bool commit(ModalityStream& stream)
    {
        if (stream.pending == 0) {
            return true;
        }
        const std::uint64_t end = stream.next + stream.pending;
        for (const Trial& t : stream.trials) {
            const std::uint64_t sample = stream.model.sampleAtOrAfter(t.onset);
            if (sample >= stream.next && sample < end) {
                const AcqError pushed =
                    stream.writer->pushEvent({sample, static_cast<std::uint32_t>(stream.model.pulse()), t.code});
                if (!pushed.ok()) {
                    reportError(pushed);
                    return false;
                }
            }
        }
        stream.writer->commit(stream.pending);
        stream.next = end;
        return true;
    }

    void run()
    {
        const std::size_t channels = m_model.channels();
        std::vector<float> frame(channels);
        std::vector<Trial> trials;
        trials.reserve(m_model.maxTrials(m_config.block));
        const Clock::time_point start = Clock::now();
        const std::uint64_t startSample = m_next;
        while (true) {
            std::size_t n = m_config.block;
            if (m_config.totalSamples > 0) {
                if (m_next >= m_config.totalSamples) {
                    m_writer->finish();
                    for (ModalityStream& stream : m_streams) {
                        stream.writer->finish();
                    }
                    return;
                }
                n = static_cast<std::size_t>(std::min<std::uint64_t>(n, m_config.totalSamples - m_next));
            }
            for (ModalityStream& stream : m_streams) {
                plan(stream, m_next + n);
            }
            if (!waitForSpace(n)) {
                return; // stopped; nothing reserved
            }
            auto view = m_writer->beginWrite(n);
            if (!view) {
                reportError(view.error());
                return;
            }
            m_model.trials(m_next, m_next + n - 1, trials);
            std::uint64_t s = m_next;
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqWritePart& part = view->part(p);
                for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                    m_model.frame(s, trials, frame.data());
                    std::memcpy(part.at(0, i), frame.data(), channels * sizeof(float));
                }
            }
            for (ModalityStream& stream : m_streams) {
                if (!generate(stream)) {
                    return;
                }
            }

            if (m_config.realtime) {
                const std::chrono::duration<double> due(static_cast<double>(m_next + n - startSample) /
                                                        (m_config.sfreq * m_config.speed));
                const auto deadline = start + std::chrono::duration_cast<Clock::duration>(due);
                std::unique_lock<std::mutex> lock(m_mutex);
                m_wake.wait_until(lock, deadline, [this] { return m_stop.load(std::memory_order_acquire); });
            }
            if (m_stop.load(std::memory_order_acquire)) {
                return; // the block is dropped in every stream; a restart regenerates it
            }

            for (const Trial& t : trials) {
                if (t.onset >= m_next && t.onset < m_next + n) {
                    const AcqError pushed = m_writer->pushEvent({t.onset, static_cast<std::uint32_t>(m_config.pulse), t.code});
                    if (!pushed.ok()) {
                        reportError(pushed);
                        return;
                    }
                }
            }
            m_writer->commit(n);
            m_next += n;
            for (ModalityStream& stream : m_streams) {
                if (!commit(stream)) {
                    return;
                }
            }
        }
    }

    Config m_config;
    Model m_model;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::uint64_t m_next = 0; // next sample index; owned by the producer thread while it runs
    std::vector<ModalityStream> m_streams;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::atomic<bool> m_stop{false};
};

//=============================================================================================================

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "synthetic";
    info.description = "Seeded oddball EEG/MEG session: noise, an alpha rhythm and evoked responses (N100, P200; "
                       "P300 on targets) with trigger events; optional fNIRS, ECoG and ERG streams at their own "
                       "rates on the same clock.";
    info.exampleUri = "synthetic://";
    info.parameters = {
        {"eeg", "int", "8", "EEG channels (V)."},
        {"meg", "int", "0", "Magnetometer channels (T)."},
        {"stim", "bool", "1", "Add the trigger channel STI 014."},
        {"sfreq", "double", "1000", "Sampling rate in Hz."},
        {"block", "int", "auto", "Samples per block; auto = 10 ms."},
        {"realtime", "bool", "1",
         "Pace blocks by the clock. 0 generates as fast as possible, so readers lose samples once they fall a "
         "whole ring behind, except a gating reader, which the session waits for (lossless replay)."},
        {"speed", "double", "1", "Pacing factor of realtime, e.g. 100 replays a session 100 times faster."},
        {"duration", "double", "0", "Session length in seconds, then end of stream; 0 runs until stopped."},
        {"seed", "int", "0", "Seed; equal seeds give bit-identical data."},
        {"cpus", "string", "", "Comma-separated CPUs for the producer thread, e.g. those of the ring's NUMA node."},
        {"priority", "string", "normal", "Producer thread priority: normal, high or realtime."},
        {"noise", "double", "1",
         "Gain of the background noise (1 = 10 uV / 200 fT rms EEG/MEG, 25 uV ECoG, 5 uV ERG; fNIRS: physiology and "
         "measurement noise)."},
        {"alpha", "double", "10", "Frequency of the background rhythm in Hz; 0 disables it."},
        {"interval", "double", "1", "Stimulus onset asynchrony in seconds (plus up to 10 % jitter); 0 disables trials."},
        {"targets", "double", "0.2", "Probability that a trial is a target."},
        {"evoked", "double", "1", "Gain of the evoked responses."},
        {"fnirs", "int", "0", "fNIRS source-detector pairs (HbO and HbR in M), stream \"fnirs\"."},
        {"fnirs_sfreq", "double", "10", "fNIRS sampling rate in whole Hz, at least 3 (cardiac pulsation at 1 Hz)."},
        {"ecog", "int", "0", "ECoG contacts (V), stream \"ecog\"."},
        {"ecog_sfreq", "double", "2000", "ECoG sampling rate in whole Hz, at least 300 (high gamma up to 140 Hz)."},
        {"erg", "int", "0", "ERG electrodes (V), stream \"erg\"."},
        {"erg_sfreq", "double", "2000", "ERG sampling rate in whole Hz, at least 300 (oscillatory potentials at 120 Hz)."},
    };
    return info;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::detail::registerSyntheticScheme(AcqDeviceRegistry& registry)
{
    return registry.registerScheme(schemeInfo(), &SyntheticBackend::open, [](std::chrono::milliseconds) {
        return std::vector<AcqDiscoveredDevice>{
            {"synthetic://", "Synthetic oddball session", "synthetic", "8 EEG + STI 014, 1000 Hz, seeded", true}};
    });
}
