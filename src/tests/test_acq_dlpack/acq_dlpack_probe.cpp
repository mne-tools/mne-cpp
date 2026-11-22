//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_dlpack_probe.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    C entry points that export a known view through DLPack, for a NumPy consumer loaded with ctypes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq_dlpack.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstring>
#include <optional>
#include <string>

#if defined(_WIN32)
#define PROBE_EXPORT extern "C" __declspec(dllexport)
#else
#define PROBE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

using namespace mne;

namespace
{
std::optional<AcqWriter> g_writer;
std::optional<AcqReader> g_reader;
AcqBlockView g_view;

/** Raw value of channel c at sample s; the Python side computes the same. */
double value(std::uint64_t s, std::size_t c)
{
    return static_cast<double>(s) * 16.0 + static_cast<double>(c) - 100.0;
}
} // namespace

/**
 * Commits @p samples samples of @p channels channels in @p format (an AcqSampleFormat value) and @p planar
 * layout, acquires them and returns the view as a DLPack tensor (nullptr on error). Holds the view until
 * acq_dlpack_probe_release().
 */
PROBE_EXPORT DLManagedTensorVersioned* acq_dlpack_probe_export(int format, int planar, int channels, int samples)
{
    AcqStreamInfo info;
    info.name = "probe";
    info.format = static_cast<AcqSampleFormat>(format);
    info.layout = planar != 0 ? AcqLayout::Planar : AcqLayout::Interleaved;
    info.sfreq = 1000.0;
    for (int c = 0; c < channels; ++c) {
        AcqChannelInfo ch;
        ch.name = "ch" + std::to_string(c);
        info.channels.push_back(ch);
    }
    AcqStreamOptions options;
    options.capacitySamples = 1024;
    options.lockMemory = false;
    auto writer = AcqWriter::create(info, options);
    if (!writer) {
        return nullptr;
    }
    g_writer.emplace(std::move(*writer));
    auto reader = g_writer->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader) {
        return nullptr;
    }
    g_reader.emplace(std::move(*reader));
    auto out = g_writer->beginWrite(static_cast<std::size_t>(samples));
    if (!out) {
        return nullptr;
    }
    const std::size_t bytes = bytesPerSample(info.format);
    for (std::size_t p = 0; p < out->partCount(); ++p) {
        const AcqWritePart& part = out->part(p);
        for (std::size_t s = 0; s < part.samples; ++s) {
            for (std::size_t c = 0; c < static_cast<std::size_t>(channels); ++c) {
                const double v = value(s, c);
                if (info.format == AcqSampleFormat::Int16) {
                    const auto x = static_cast<std::int16_t>(v);
                    std::memcpy(part.at(c, s), &x, bytes);
                } else if (info.format == AcqSampleFormat::Int32) {
                    const auto x = static_cast<std::int32_t>(v);
                    std::memcpy(part.at(c, s), &x, bytes);
                } else if (info.format == AcqSampleFormat::Float32) {
                    const auto x = static_cast<float>(v);
                    std::memcpy(part.at(c, s), &x, bytes);
                } else if (info.format == AcqSampleFormat::Float64) {
                    std::memcpy(part.at(c, s), &v, bytes);
                } else {
                    const float x[2] = {static_cast<float>(v), static_cast<float>(-v)};
                    std::memcpy(part.at(c, s), x, bytes);
                }
            }
        }
    }
    g_writer->commit(static_cast<std::size_t>(samples));
    auto view = g_reader->acquire(static_cast<std::size_t>(samples), std::chrono::nanoseconds(0));
    if (!view) {
        return nullptr;
    }
    g_view = *view;
    auto tensor = toDLPack(g_view);
    return tensor ? *tensor : nullptr;
}

/** Releases the held view; @return 0 on success. */
PROBE_EXPORT int acq_dlpack_probe_release()
{
    const int result = g_reader && g_reader->release(g_view).ok() ? 0 : 1;
    g_reader.reset();
    g_writer.reset();
    return result;
}
