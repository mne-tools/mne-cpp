//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Add tags from one FIFF file to the meas_info block of another.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Add tags from one FIFF file to the meas_info block of another.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --add <name>    FIFF file containing tags to add\n");
    fprintf(stderr, "  --dest <name>   Destination FIFF file to receive additional tags\n");
    fprintf(stderr, "  --help          Print this help\n");
    fprintf(stderr, "  --version       Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString addName;
    QString destName;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        } else if (strcmp(argv[k], "--add") == 0) {
            if (k + 1 >= argc) { qCritical("--add: argument required."); return 1; }
            addName = QString(argv[++k]);
        } else if (strcmp(argv[k], "--dest") == 0) {
            if (k + 1 >= argc) { qCritical("--dest: argument required."); return 1; }
            destName = QString(argv[++k]);
        } else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (addName.isEmpty() || destName.isEmpty()) {
        qCritical("Both --add and --dest are required.");
        usage(argv[0]);
        return 1;
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

    QList<FiffTag::SPtr> tagsToAdd;
    for (int k = 0; k < addStream->nent(); k++) {
        fiff_int_t kind = addStream->dir()[k]->kind;
        // Skip file-level structural tags
        if (kind == FIFF_FILE_ID || kind == FIFF_DIR_POINTER ||
            kind == FIFF_FREE_LIST || kind == FIFF_NOP ||
            kind == FIFF_BLOCK_START || kind == FIFF_BLOCK_END) {
            continue;
        }
        FiffTag::SPtr tag;
        addStream->read_tag(tag, addStream->dir()[k]->pos);
        tagsToAdd.append(tag);
    }
    addStream->close();

    fprintf(stderr, "%d tags to add\n", tagsToAdd.size());

    if (tagsToAdd.isEmpty()) {
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
    for (const FiffTag::SPtr &tag : tagsToAdd) {
        destStream->write_tag(tag);
        nAdded++;
    }

    destStream->close();

    fprintf(stderr, "%d tags added to %s\n", nAdded, qPrintable(destName));
    fprintf(stderr, "done.\n");

    return 0;
}
