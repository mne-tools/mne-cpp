//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Mark or update bad channels in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static StringList readBadChannelList(const QString& filename)
{
    StringList bads;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open bad channel file: {}", filename.toStdString());
        return bads;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#'))
            bads.push_back(line.toStdString());
    }
    return bads;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
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
        Log::critical("Both --fif and --bad are required.");
        return 1;
    }
    if (!QFileInfo(badName).isReadable()) {
        Log::critical("Cannot open bad channel file: {}", badName.toStdString());
        return 1;
    }
    if (!QFileInfo(fifName).isReadable()) {
        Log::critical("Cannot read FIFF file: {}", fifName.toStdString());
        return 1;
    }

    // Read the bad channel list
    StringList bads = readBadChannelList(badName);
    if (bads.empty()) {
        Log::warning("Bad channel list is empty.");
    }

    Log::info("Bad channels to mark ({}):", static_cast<long long>(bads.size()));
    for (const std::string& ch : bads) {
        Log::info("  {}", ch);
    }

    // Open the FIFF file and read raw data
    FiffRawData raw;
    try {
        raw = FiffRawData(fifName.toStdU16String());
    } catch (const std::exception& error) {
        Log::critical("Cannot read FIFF file {}: {}", fifName.toStdString(), error.what());
        return 1;
    }
    if (raw.info.empty()) {
        Log::critical("Cannot read FIFF file: {}", fifName.toStdString());
        return 1;
    }

    // Update bad channels
    Log::info("Previous bad channels: {}", static_cast<long long>(raw.info.bads.size()));
    raw.info.bads = bads;
    Log::info("New bad channels: {}", static_cast<long long>(raw.info.bads.size()));

    // Read all data
    Eigen::MatrixXd data;
    Eigen::MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        Log::critical("Cannot read raw data segment.");
        return 1;
    }

    // Write to temp file, then replace
    QString tmpName = fifName + ".tmp";
    Eigen::RowVectorXd cals;
    auto outStream = FiffStream::start_writing_raw(tmpName.toStdU16String(), raw.info, cals);
    if (!outStream) {
        Log::critical("Cannot open temp file for writing.");
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

    Log::info("Successfully updated bad channels in {}", fifName.toStdString());

    return 0;
}
