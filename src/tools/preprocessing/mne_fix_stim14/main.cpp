//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Fixes the trigger channels of raw FIFF files in place, as MNE-C's mne_fix_stim14 does.
 *
 * By default STI 014 gets the range 1.0 and keeps only the low byte of its 16-bit samples; with --32 STI101
 * and STI201 keep the low 16 bits of their 32-bit integer or float samples. A file is checked completely
 * before anything in it changes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_name.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

namespace
{

constexpr int kRangeOffset = 3 * sizeof(fiff_int_t); // scanNo, logNo, kind, then range in a channel info

/** Rewrites sample @p channel of every frame in @p tag with @p fix; @return the number of samples changed. */
template<typename T, typename Fix>
int fixSamples(FiffTag& tag, int channel, int nchan, Fix fix)
{
    int changed = 0;
    std::byte* data = tag.data();
    const int frames = static_cast<int>(tag.size() / (sizeof(T) * nchan));
    for (int frame = 0; frame < frames; ++frame) {
        std::byte* at = data + (static_cast<std::size_t>(frame) * nchan + channel) * sizeof(T);
        T value;
        std::memcpy(&value, at, sizeof(T));
        const T fixed = fix(value);
        if (fixed != value) {
            std::memcpy(at, &fixed, sizeof(T));
            ++changed;
        }
    }
    return changed;
}

/** Fixes @p path in place; @return the error, empty on success. */
std::string fixFile(const QString& path, bool fix32)
{
    QFile file(path);
    auto stream = FiffStream::open_update(path.toStdU16String());
    if (!stream) {
        return "cannot open the file for update";
    }
    FiffInfo info;
    std::shared_ptr<FiffDirNode> infoNode;
    if (!stream->read_meas_info(stream->dirtree(), info, infoNode)) {
        return "no measurement info";
    }

    // Check everything first, so a refused file stays as it was
    const std::vector<int> channels = fix32 ? std::vector<int>{channel_index(info.ch_names, "STI101"), channel_index(info.ch_names, "STI201")}
                                            : std::vector<int>{channel_index(info.ch_names, "STI 014")};
    if (channels.front() < 0 && channels.back() < 0) {
        return fix32 ? "Neither STI101 nor STI201 found" : "Digital trigger channel STI 014 not found";
    }
    std::vector<FiffDirEntry> buffers;
    for (const FiffDirEntry& entry : stream->dir()) {
        if (entry.kind != FIFF_DATA_BUFFER) {
            continue;
        }
        const bool fits = fix32 ? entry.type == FIFFT_INT || entry.type == FIFFT_FLOAT : entry.type == FIFFT_SHORT || entry.type == FIFFT_DAU_PACK16;
        if (!fits) {
            return fix32 ? "--32 fixes 32-bit integer and float buffers only" : "STI 014 can be fixed in 16-bit buffers only";
        }
        buffers.push_back(entry);
    }

    if (!fix32) {
        for (const FiffDirEntry& entry : stream->dir()) {
            if (entry.kind != FIFF_CH_INFO) {
                continue;
            }
            std::optional<FiffTag> tag = stream->read_tag(entry.pos);
            if (!tag || !same_channel(tag->to_ch_info().ch_name, "STI 014") || tag->to_ch_info().range == 1.0f) {
                continue;
            }
            const float range = 1.0f;
            std::memcpy(tag->data() + kRangeOffset, &range, sizeof(range));
            stream->write_tag(*tag, entry.pos);
            Log::info("  STI 014 range set to 1");
        }
    }

    int changed = 0;
    for (const FiffDirEntry& entry : buffers) {
        std::optional<FiffTag> tag = stream->read_tag(entry.pos);
        if (!tag) {
            return "cannot read a data buffer";
        }
        int inTag = 0;
        for (const int channel : channels) {
            if (channel < 0) {
                continue;
            }
            if (entry.type == FIFFT_INT) {
                inTag += fixSamples<std::int32_t>(*tag, channel, info.nchan, [](std::int32_t v) { return static_cast<std::int32_t>(static_cast<std::uint16_t>(v)); });
            } else if (entry.type == FIFFT_FLOAT) {
                inTag += fixSamples<float>(*tag, channel, info.nchan, [](float v) { return static_cast<float>(static_cast<std::uint16_t>(static_cast<std::int32_t>(v))); });
            } else {
                inTag += fixSamples<std::int16_t>(*tag, channel, info.nchan, [](std::int16_t v) { return static_cast<std::int16_t>(static_cast<std::uint16_t>(v) & 0xFF); });
            }
        }
        if (inTag > 0) {
            stream->write_tag(*tag, entry.pos);
            changed += inTag;
        }
    }
    Log::info("  {} samples fixed", changed);
    stream->close();
    return {};
}

} // namespace

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_fix_stim14");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Fix the trigger channels of raw FIFF files in place.\n\n"
                                     "STI 014 gets the range 1 and keeps only the low byte of its samples.\n"
                                     "With --32, STI101 and STI201 keep only the low 16 bits of their 32-bit samples.");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption fix32Opt("32", "Keep only the two lowest bytes of STI101 and STI201 (32-bit integer and float data).");
    parser.addOption(fix32Opt);
    parser.addPositionalArgument("files", "Raw FIFF files, fixed in place.", "<file>...");
    parser.process(app);

    const QStringList files = parser.positionalArguments();
    if (files.isEmpty()) {
        parser.showHelp(1);
    }
    int failed = 0;
    for (const QString& path : files) {
        Log::info("{}", path.toStdString());
        if (const std::string error = fixFile(path, parser.isSet(fix32Opt)); !error.empty()) {
            Log::critical("  {}: {}", path.toStdString(), error);
            ++failed;
        }
    }
    return failed == 0 ? 0 : 1;
}
