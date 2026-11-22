//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Add trigger events to raw data's STI channel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct TriggerEvent
{
    int sample;
    int value;
};

//=============================================================================================================

static QList<TriggerEvent> readTriggerFile(const QString& filename)
{
    QList<TriggerEvent> triggers;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open trigger file: {}", filename.toStdString());
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

int main(int argc, char* argv[])
{
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

    if (rawFile.isEmpty()) {
        Log::critical("--raw is required.");
        return 1;
    }
    if (trgFile.isEmpty()) {
        Log::critical("--trg is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Read trigger file
    QList<TriggerEvent> triggers = readTriggerFile(trgFile);
    if (triggers.isEmpty()) {
        Log::critical("No triggers read from: {}", trgFile.toStdString());
        return 1;
    }
    Log::info("Read {} trigger events from {}", static_cast<long long>(triggers.size()), trgFile.toStdString());

    // Open raw data
    FiffRawData raw(rawFile.toStdU16String());
    if (raw.info.empty()) {
        Log::critical("Cannot read raw data from: {}", rawFile.toStdString());
        return 1;
    }

    // Find STI channel (prefer STI 014, fallback to first stim channel)
    int stiIdx = -1;
    for (int i = 0; i < std::ssize(raw.info.chs); ++i) {
        if ((raw.info.chs[i].ch_name.find("STI") != std::string::npos) ||
            raw.info.chs[i].kind == FIFFV_STIM_CH) {
            if ((raw.info.chs[i].ch_name.find("014") != std::string::npos) || stiIdx < 0) {
                stiIdx = i;
            }
        }
    }

    if (stiIdx < 0) {
        Log::critical("No STI/stimulus channel found in data.");
        return 1;
    }
    Log::info("Using stimulus channel: {} (index {})", raw.info.chs[stiIdx].ch_name, stiIdx);

    // Read all data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        Log::critical("Cannot read raw data segment.");
        return 1;
    }

    // Add triggers
    int nAdded = 0;
    for (const TriggerEvent& evt : triggers) {
        int col = evt.sample - raw.first_samp;
        if (col >= 0 && col < data.cols()) {
            data(stiIdx, col) = static_cast<double>(evt.value);
            ++nAdded;
        } else {
            Log::warning("Trigger sample {} out of range [{}, {}], skipping.", evt.sample, raw.first_samp, raw.last_samp);
        }
    }
    Log::info("Added {} trigger events.", nAdded);

    // Write output
    RowVectorXd cals;
    auto outStream = FiffStream::start_writing_raw(outFile.toStdU16String(), raw.info, cals);
    if (!outStream) {
        Log::critical("Cannot open output file: {}", outFile.toStdString());
        return 1;
    }

    fiff_int_t firstSample = raw.first_samp;
    outStream->write_int(FIFF_FIRST_SAMPLE, &firstSample);

    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk, cals);
    }
    outStream->finish_writing_raw();

    Log::info("Written modified raw data to: {}", outFile.toStdString());
    return 0;
}
