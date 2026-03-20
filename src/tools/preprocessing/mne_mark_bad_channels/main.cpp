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
 * @brief    Mark or update bad channels in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

static QStringList readBadChannelList(const QString &filename)
{
    QStringList bads;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open bad channel file: %s", qPrintable(filename));
        return bads;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#'))
            bads.append(line);
    }
    return bads;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_mark_bad_channels");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Mark or update bad channels in a FIFF file.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption badOpt("bad", "Text file listing bad channel names (one per line).", "file");
    parser.addOption(badOpt);

    QCommandLineOption fifOpt("fif", "FIFF file to modify.", "file");
    parser.addOption(fifOpt);

    parser.process(app);

    QString badName = parser.value(badOpt);
    QString fifName = parser.value(fifOpt);

    if (fifName.isEmpty() || badName.isEmpty()) {
        qCritical("Both --fif and --bad are required.");
        return 1;
    }

    // Read the bad channel list
    QStringList bads = readBadChannelList(badName);
    if (bads.isEmpty()) {
        qWarning("Bad channel list is empty.");
    }

    printf("Bad channels to mark (%lld):\n", static_cast<long long>(bads.size()));
    for (const QString &ch : bads) {
        printf("  %s\n", qPrintable(ch));
    }

    // Open the FIFF file and read raw data
    QFile file(fifName);
    FiffRawData raw(file);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read FIFF file: %s", qPrintable(fifName));
        return 1;
    }

    // Update bad channels
    printf("Previous bad channels: %lld\n", static_cast<long long>(raw.info.bads.size()));
    raw.info.bads = bads;
    printf("New bad channels: %lld\n", static_cast<long long>(raw.info.bads.size()));

    // Read all data
    Eigen::MatrixXd data;
    Eigen::MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        qCritical("Cannot read raw data segment.");
        return 1;
    }

    // Write to temp file, then replace
    QString tmpName = fifName + ".tmp";
    QFile outFile(tmpName);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(outFile, raw.info, cals);
    if (!outStream) {
        qCritical("Cannot open temp file for writing.");
        return 1;
    }

    // Write data in chunks
    int nSamples = data.cols();
    int nChan = data.rows();
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        Eigen::MatrixXd chunk = data.block(0, start, nChan, end - start);
        outStream->write_raw_buffer(chunk);
    }

    outStream->finish_writing_raw();

    // Replace original with temp
    QFile::remove(fifName);
    QFile::rename(tmpName, fifName);

    printf("Successfully updated bad channels in %s\n", qPrintable(fifName));

    return 0;
}
