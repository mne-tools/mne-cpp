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
 * @brief    Compare two FIFF files tag by tag.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_constants.h>
#include <utils/generics/applicationlogger.h>

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

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static QString tagName(int kind)
{
    // Return a human-readable tag name for common FIFF tags
    switch (kind) {
        case FIFF_FILE_ID:          return "FILE_ID";
        case FIFF_DIR_POINTER:      return "DIR_POINTER";
        case FIFF_DIR:              return "DIR";
        case FIFF_BLOCK_ID:         return "BLOCK_ID";
        case FIFF_BLOCK_START:      return "BLOCK_START";
        case FIFF_BLOCK_END:        return "BLOCK_END";
        case FIFF_FREE_LIST:        return "FREE_LIST";
        case FIFF_NCHAN:            return "NCHAN";
        case FIFF_SFREQ:            return "SFREQ";
        case FIFF_CH_INFO:          return "CH_INFO";
        case FIFF_MEAS_DATE:        return "MEAS_DATE";
        case FIFF_COORD_TRANS:      return "COORD_TRANS";
        case FIFF_NAVE:             return "NAVE";
        case FIFF_FIRST_SAMPLE:     return "FIRST_SAMPLE";
        case FIFF_LAST_SAMPLE:      return "LAST_SAMPLE";
        case FIFF_COMMENT:          return "COMMENT";
        default:                    return QString("TAG_%1").arg(kind);
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
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
        qCritical("Both --file1 and --file2 are required.");
        return 1;
    }

    // Open both files
    QFile f1(file1);
    if (!f1.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open: %s", qPrintable(file1));
        return 1;
    }

    QFile f2(file2);
    if (!f2.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open: %s", qPrintable(file2));
        return 1;
    }

    FiffStream::SPtr stream1(new FiffStream(&f1));
    FiffStream::SPtr stream2(new FiffStream(&f2));

    if (!stream1->open()) {
        qCritical("Cannot parse FIFF: %s", qPrintable(file1));
        return 1;
    }

    if (!stream2->open()) {
        qCritical("Cannot parse FIFF: %s", qPrintable(file2));
        return 1;
    }

    const QList<FiffDirEntry::SPtr>& dir1 = stream1->dir();
    const QList<FiffDirEntry::SPtr>& dir2 = stream2->dir();

    printf("File 1: %s (%d directory entries)\n", qPrintable(file1), (int)dir1.size());
    printf("File 2: %s (%d directory entries)\n", qPrintable(file2), (int)dir2.size());
    printf("\n");

    int nMatch = 0, nDiff = 0, nMissing = 0;
    int maxEntries = (int)std::max(dir1.size(), dir2.size());
    int minEntries = (int)std::min(dir1.size(), dir2.size());

    for (int i = 0; i < minEntries; ++i) {
        FiffTag::SPtr tag1, tag2;
        stream1->read_tag(tag1, dir1[i]->pos);
        stream2->read_tag(tag2, dir2[i]->pos);

        bool kindMatch = (dir1[i]->kind == dir2[i]->kind);
        bool typeMatch = (dir1[i]->type == dir2[i]->type);
        bool sizeMatch = (dir1[i]->size == dir2[i]->size);

        bool dataMatch = false;
        if (sizeMatch && tag1 && tag2) {
            dataMatch = (tag1->data() == tag2->data());
        }

        bool isMatch = kindMatch && typeMatch && sizeMatch && dataMatch;

        if (isMatch) {
            nMatch++;
            if (verbose)
                printf("  [%4d] %s : MATCH (size=%d)\n",
                       i, qPrintable(tagName(dir1[i]->kind)), dir1[i]->size);
        } else {
            nDiff++;
            printf("  [%4d] DIFF:", i);
            if (!kindMatch)
                printf(" kind=%d vs %d", dir1[i]->kind, dir2[i]->kind);
            else
                printf(" kind=%s", qPrintable(tagName(dir1[i]->kind)));
            if (!typeMatch) printf(" type=%d vs %d", dir1[i]->type, dir2[i]->type);
            if (!sizeMatch) printf(" size=%d vs %d", dir1[i]->size, dir2[i]->size);
            if (sizeMatch && !dataMatch) printf(" DATA_DIFFERS");
            printf("\n");
        }
    }

    if (dir1.size() != dir2.size()) {
        nMissing = (int)abs(dir1.size() - dir2.size());
        printf("\n  File %s has %d extra directory entries.\n",
               dir1.size() > dir2.size() ? "1" : "2", nMissing);
    }

    printf("\nSummary: %d matching, %d different, %d extra entries\n",
           nMatch, nDiff, nMissing);

    bool identical = (nDiff == 0 && nMissing == 0);
    printf("Files are %s.\n", identical ? "IDENTICAL" : "DIFFERENT");

    stream1->close();
    stream2->close();

    return identical ? 0 : 1;
}
