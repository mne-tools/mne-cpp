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
 * @brief    Add trigger events to raw data's STI channel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
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
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct TriggerEvent {
    int sample;
    int value;
};

//=============================================================================================================

static QList<TriggerEvent> readTriggerFile(const QString &filename)
{
    QList<TriggerEvent> triggers;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open trigger file: %s", qPrintable(filename));
        return triggers;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 2) {
            TriggerEvent evt;
            evt.sample = parts[0].toInt();
            evt.value = parts[1].toInt();
            triggers.append(evt);
        }
    }
    return triggers;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_add_triggers");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Add trigger events to raw data's STI channel.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOpt("raw", "Input raw FIFF file.", "file");
    parser.addOption(rawOpt);

    QCommandLineOption trgOpt("trg", "Trigger file (sample event_value per line).", "file");
    parser.addOption(trgOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString rawFile = parser.value(rawOpt);
    QString trgFile = parser.value(trgOpt);
    QString outFile = parser.value(outOpt);

    if (rawFile.isEmpty()) { qCritical("--raw is required."); return 1; }
    if (trgFile.isEmpty()) { qCritical("--trg is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Read trigger file
    QList<TriggerEvent> triggers = readTriggerFile(trgFile);
    if (triggers.isEmpty()) {
        qCritical("No triggers read from: %s", qPrintable(trgFile));
        return 1;
    }
    printf("Read %lld trigger events from %s\n",
           static_cast<long long>(triggers.size()), qPrintable(trgFile));

    // Open raw data
    QFile fileIn(rawFile);
    FiffRawData raw(fileIn);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw data from: %s", qPrintable(rawFile));
        return 1;
    }

    // Find STI channel (prefer STI 014, fallback to first stim channel)
    int stiIdx = -1;
    for (int i = 0; i < raw.info.chs.size(); ++i) {
        if (raw.info.chs[i].ch_name.contains("STI") ||
            raw.info.chs[i].kind == FIFFV_STIM_CH) {
            if (raw.info.chs[i].ch_name.contains("014") || stiIdx < 0) {
                stiIdx = i;
            }
        }
    }

    if (stiIdx < 0) {
        qCritical("No STI/stimulus channel found in data.");
        return 1;
    }
    printf("Using stimulus channel: %s (index %d)\n",
           qPrintable(raw.info.chs[stiIdx].ch_name), stiIdx);

    // Read all data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        qCritical("Cannot read raw data segment.");
        return 1;
    }

    // Add triggers
    int nAdded = 0;
    for (const TriggerEvent &evt : triggers) {
        int col = evt.sample - raw.first_samp;
        if (col >= 0 && col < data.cols()) {
            data(stiIdx, col) = static_cast<double>(evt.value);
            ++nAdded;
        } else {
            qWarning("Trigger sample %d out of range [%d, %d], skipping.",
                     evt.sample, raw.first_samp, raw.last_samp);
        }
    }
    printf("Added %d trigger events.\n", nAdded);

    // Write output
    QFile fileOut(outFile);
    RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(fileOut, raw.info, cals);
    if (!outStream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk);
    }
    outStream->finish_writing_raw();

    printf("Written modified raw data to: %s\n", qPrintable(outFile));
    return 0;
}
