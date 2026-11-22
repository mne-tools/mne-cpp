//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Compare two FIFF files tag by tag.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString tagName(int kind)
{
    // Return a human-readable tag name for common FIFF tags
    switch (kind) {
        case FIFF_FILE_ID:
            return "FILE_ID";
        case FIFF_DIR_POINTER:
            return "DIR_POINTER";
        case FIFF_DIR:
            return "DIR";
        case FIFF_BLOCK_ID:
            return "BLOCK_ID";
        case FIFF_BLOCK_START:
            return "BLOCK_START";
        case FIFF_BLOCK_END:
            return "BLOCK_END";
        case FIFF_FREE_LIST:
            return "FREE_LIST";
        case FIFF_NCHAN:
            return "NCHAN";
        case FIFF_SFREQ:
            return "SFREQ";
        case FIFF_CH_INFO:
            return "CH_INFO";
        case FIFF_MEAS_DATE:
            return "MEAS_DATE";
        case FIFF_COORD_TRANS:
            return "COORD_TRANS";
        case FIFF_NAVE:
            return "NAVE";
        case FIFF_FIRST_SAMPLE:
            return "FIRST_SAMPLE";
        case FIFF_LAST_SAMPLE:
            return "LAST_SAMPLE";
        case FIFF_COMMENT:
            return "COMMENT";
        default:
            return QString("TAG_%1").arg(kind);
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_compare_fif_files");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compare two FIFF files tag by tag.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption file1Opt("file1", "First FIFF file.", "file");
    parser.addOption(file1Opt);

    QCommandLineOption file2Opt("file2", "Second FIFF file.", "file");
    parser.addOption(file2Opt);

    QCommandLineOption verboseOpt("verbose", "Show matching tags too.");
    parser.addOption(verboseOpt);

    parser.process(app);

    QString file1 = parser.value(file1Opt);
    QString file2 = parser.value(file2Opt);
    bool verbose = parser.isSet(verboseOpt);

    if (file1.isEmpty() || file2.isEmpty()) {
        Log::critical("Both --file1 and --file2 are required.");
        return 1;
    }

    // Open both files
    FiffStream stream1(file1.toStdU16String());
    FiffStream stream2(file2.toStdU16String());

    if (!stream1.open()) {
        Log::critical("Cannot parse FIFF: {}", file1.toStdString());
        return 1;
    }

    if (!stream2.open()) {
        Log::critical("Cannot parse FIFF: {}", file2.toStdString());
        return 1;
    }

    const std::vector<FiffDirEntry>& dir1 = stream1.dir();
    const std::vector<FiffDirEntry>& dir2 = stream2.dir();

    Log::info("File 1: {} ({} directory entries)", file1.toStdString(), static_cast<int>(dir1.size()));
    Log::info("File 2: {} ({} directory entries)", file2.toStdString(), static_cast<int>(dir2.size()));
    Log::info("{}", "");

    int nMatch = 0, nDiff = 0, nMissing = 0;
    int minEntries = static_cast<int>(std::min(dir1.size(), dir2.size()));

    for (int i = 0; i < minEntries; ++i) {
        // The directory ends with an entry that marks its end, not a tag.
        if (dir1[i].pos < 0 && dir2[i].pos < 0) {
            continue;
        }
        const std::optional<FiffTag> tag1 = stream1.read_tag(dir1[i].pos);
        const std::optional<FiffTag> tag2 = stream2.read_tag(dir2[i].pos);

        bool kindMatch = (dir1[i].kind == dir2[i].kind);
        bool typeMatch = (dir1[i].type == dir2[i].type);
        bool sizeMatch = (dir1[i].size == dir2[i].size);

        bool dataMatch = false;
        if (sizeMatch && tag1 && tag2) {
            dataMatch = std::ranges::equal(tag1->bytes(), tag2->bytes());
        }

        bool isMatch = kindMatch && typeMatch && sizeMatch && dataMatch;

        if (isMatch) {
            nMatch++;
            if (verbose)
                Log::info("  [{:4}] {} : MATCH (size={})", i, tagName(dir1[i].kind).toStdString(), dir1[i].size);
        } else {
            nDiff++;
            QString line = QString::asprintf("  [%4d] DIFF:", i);
            if (!kindMatch)
                line += QString::asprintf(" kind=%d vs %d", dir1[i].kind, dir2[i].kind);
            else
                line += " kind=" + tagName(dir1[i].kind);
            if (!typeMatch)
                line += QString::asprintf(" type=%d vs %d", dir1[i].type, dir2[i].type);
            if (!sizeMatch)
                line += QString::asprintf(" size=%d vs %d", dir1[i].size, dir2[i].size);
            if (sizeMatch && !dataMatch)
                line += " DATA_DIFFERS";
            Log::info("{}", line.toStdString());
        }
    }

    if (dir1.size() != dir2.size()) {
        nMissing = static_cast<int>(std::abs(std::ssize(dir1) - std::ssize(dir2)));
        Log::info("\n  File {} has {} extra directory entries.", dir1.size() > dir2.size() ? "1" : "2", nMissing);
    }

    Log::info("\nSummary: {} matching, {} different, {} extra entries", nMatch, nDiff, nMissing);

    bool identical = (nDiff == 0 && nMissing == 0);
    Log::info("Files are {}.", identical ? "IDENTICAL" : "DIFFERENT");

    stream1.close();
    stream2.close();

    return identical ? 0 : 1;
}
