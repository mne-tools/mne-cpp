//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Toggle skip tags in raw data (in-place).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_types.h>
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
    if (rawFile.isEmpty()) { qCritical("--raw is required."); return 1; }

    // Open file for update
    QFile file(rawFile);
    FiffStream::SPtr stream = FiffStream::open_update(file);
    if (!stream) {
        qCritical("Cannot open FIFF file for update: %s", qPrintable(rawFile));
        return 1;
    }

    // Scan directory for FIFF_DATA_SKIP tags
    int nToggled = 0;
    const QList<FiffDirEntry::SPtr> &dirEntries = stream->dir();

    for (int i = 0; i < dirEntries.size(); ++i) {
        if (dirEntries[i]->kind == FIFF_DATA_SKIP) {
            // Read the tag
            std::unique_ptr<FiffTag> tag;
            if (!stream->read_tag(tag, dirEntries[i]->pos)) {
                qWarning("Cannot read tag at position %lld, skipping.",
                         static_cast<long long>(dirEntries[i]->pos));
                continue;
            }

            // Toggle: if the skip count is positive, negate it (disable skip);
            // if negative, make it positive (enable skip)
            fiff_int_t skipCount = *tag->toInt();
            fiff_int_t newSkipCount = -skipCount;

            // Write back the modified skip count at the data position
            // The data starts after the tag header (12 bytes)
            fiff_long_t dataPos = dirEntries[i]->pos + 16; // kind(4) + type(4) + size(4) + next(4)
            stream->device()->seek(dataPos);
            fiff_int_t beVal = newSkipCount;
            // Write as big-endian int32
            stream->writeRawData(reinterpret_cast<const char*>(&beVal), sizeof(fiff_int_t));

            printf("  Tag at pos %lld: skip count %d -> %d\n",
                   static_cast<long long>(dirEntries[i]->pos), skipCount, newSkipCount);
            ++nToggled;
        }
    }

    stream->close();

    if (nToggled == 0) {
        printf("No FIFF_DATA_SKIP tags found in %s\n", qPrintable(rawFile));
    } else {
        printf("Toggled %d skip tag(s) in %s\n", nToggled, qPrintable(rawFile));
    }

    return 0;
}
