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

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>

#include <vector>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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
        qCritical("Both --add and --dest are required.");
        parser.showHelp(1);
    }

    fprintf(stderr, "Source file      : %s\n", qPrintable(addName));
    fprintf(stderr, "Destination file : %s\n", qPrintable(destName));

    // Read all tags from the source file
    QFile addFile(addName);
    FiffStream::SPtr addStream(new FiffStream(&addFile));
    if (!addStream->open()) {
        qCritical("Cannot open source file: %s", qPrintable(addName));
        return 1;
    }

    std::vector<FiffTag::UPtr> tagsToAdd;
    for (int k = 0; k < addStream->nent(); k++) {
        fiff_int_t kind = addStream->dir()[k]->kind;
        // Skip file-level structural tags
        if (kind == FIFF_FILE_ID || kind == FIFF_DIR_POINTER ||
            kind == FIFF_FREE_LIST || kind == FIFF_NOP ||
            kind == FIFF_BLOCK_START || kind == FIFF_BLOCK_END) {
            continue;
        }
        FiffTag::UPtr tag;
        addStream->read_tag(tag, addStream->dir()[k]->pos);
        tagsToAdd.push_back(std::move(tag));
    }
    addStream->close();

    fprintf(stderr, "%d tags to add\n", static_cast<int>(tagsToAdd.size()));

    if (tagsToAdd.empty()) {
        fprintf(stderr, "No tags to add.\n");
        return 0;
    }

    // Open destination file for update and append tags
    QFile destFile(destName);
    FiffStream::SPtr destStream = FiffStream::open_update(destFile);
    if (!destStream) {
        qCritical("Cannot open destination file for update: %s", qPrintable(destName));
        return 1;
    }

    // Append all tags at the end
    int nAdded = 0;
    for (const FiffTag::UPtr &tag : tagsToAdd) {
        destStream->write_tag(tag);
        nAdded++;
    }

    destStream->close();

    fprintf(stderr, "%d tags added to %s\n", nAdded, qPrintable(destName));
    fprintf(stderr, "done.\n");

    return 0;
}
