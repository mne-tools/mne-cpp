//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Add tags from one FIFF file to the meas_info block of another.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>
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

#include <vector>

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
    QCoreApplication::setApplicationName("mne_add_to_meas_info");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Add tags from one FIFF file to the meas_info block of another.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption addOpt("add", "FIFF file containing tags to add.", "name");
    parser.addOption(addOpt);

    QCommandLineOption destOpt("dest", "Destination FIFF file to receive additional tags.", "name");
    parser.addOption(destOpt);

    parser.process(app);

    QString addName = parser.value(addOpt);
    QString destName = parser.value(destOpt);

    if (addName.isEmpty() || destName.isEmpty()) {
        Log::critical("Both --add and --dest are required.");
        parser.showHelp(1);
    }

    Log::info("Source file      : {}", addName.toStdString());
    Log::info("Destination file : {}", destName.toStdString());

    // Read all tags from the source file
    FiffStream addStream(addName.toStdU16String());
    if (!addStream.open()) {
        Log::critical("Cannot open source file: {}", addName.toStdString());
        return 1;
    }

    std::vector<FiffTag> tagsToAdd;
    for (int k = 0; k < addStream.nent(); k++) {
        fiff_int_t kind = addStream.dir()[k].kind;
        // Skip file-level structural tags
        if (kind == FIFF_FILE_ID || kind == FIFF_DIR_POINTER ||
            kind == FIFF_FREE_LIST || kind == FIFF_NOP ||
            kind == FIFF_BLOCK_START || kind == FIFF_BLOCK_END) {
            continue;
        }
        if (auto tag = addStream.read_tag(addStream.dir()[k].pos)) {
            tagsToAdd.push_back(std::move(*tag));
        }
    }
    addStream.close();

    Log::info("{} tags to add", static_cast<int>(tagsToAdd.size()));

    if (tagsToAdd.empty()) {
        Log::info("No tags to add.");
        return 0;
    }

    // Open destination file for update and append tags
    auto destStream = FiffStream::open_update(destName.toStdU16String());
    if (!destStream) {
        Log::critical("Cannot open destination file for update: {}", destName.toStdString());
        return 1;
    }

    // Append all tags at the end
    int nAdded = 0;
    for (const FiffTag& tag : tagsToAdd) {
        destStream->write_tag(tag);
        nAdded++;
    }

    destStream->close();

    Log::info("{} tags added to {}", nAdded, destName.toStdString());
    Log::info("done.");

    return 0;
}
