//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Toggle skip tags in raw data (in-place).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_types.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_toggle_skips");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Toggle skip tags in raw FIFF data.\n\n"
                                     "This modifies the file in-place, toggling FIFF_DATA_SKIP tags\n"
                                     "between active (skip) and inactive (unskip) states.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOpt("raw", "Input raw FIFF file (modified in-place).", "file");
    parser.addOption(rawOpt);

    parser.process(app);

    QString rawFile = parser.value(rawOpt);
    if (rawFile.isEmpty()) {
        Log::critical("--raw is required.");
        return 1;
    }

    // Open file for update
    QFile file(rawFile);
    auto stream = FiffStream::open_update(rawFile.toStdU16String());
    if (!stream) {
        Log::critical("Cannot open FIFF file for update: {}", rawFile.toStdString());
        return 1;
    }

    // As MNE-C: a skip is turned off by renaming it FIFF_MNE_DATA_SKIP_NOP, which readers ignore, and on again
    int turnedOff = 0;
    int turnedOn = 0;
    for (const FiffDirEntry& entry : stream->dir()) {
        if (entry.kind != FIFF_DATA_SKIP && entry.kind != FIFF_MNE_DATA_SKIP_NOP) {
            continue;
        }
        std::optional<FiffTag> tag = stream->read_tag(entry.pos);
        if (!tag) {
            Log::critical("Cannot read the tag at position {}.", static_cast<long long>(entry.pos));
            return 1;
        }
        const bool on = tag->kind == FIFF_DATA_SKIP;
        tag->kind = on ? FIFF_MNE_DATA_SKIP_NOP : FIFF_DATA_SKIP;
        stream->write_tag(*tag, entry.pos);
        ++(on ? turnedOff : turnedOn);
    }
    stream->close();

    Log::info("{} skips turned off, {} skips turned on in {}", turnedOff, turnedOn, rawFile.toStdString());
    return 0;
}
