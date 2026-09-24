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

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
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
    qInstallMessageHandler(MNELogger::customLogWriter);
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
    if (!QFileInfo(badName).isReadable()) {
        qCritical("Cannot open bad channel file: %s", qPrintable(badName));
        return 1;
    }
    if (!QFileInfo(fifName).isReadable()) {
        qCritical("Cannot read FIFF file: %s", qPrintable(fifName));
        return 1;
    }

    // Read the bad channel list
    QStringList bads = readBadChannelList(badName);
    if (bads.isEmpty()) {
        qWarning("Bad channel list is empty.");
    }

    qInfo("Bad channels to mark (%lld):" , static_cast<long long>(bads.size()));
    for (const QString &ch : bads) {
        qInfo("  %s" , qPrintable(ch));
    }

    // Open the FIFF file and read raw data
    QFile file(fifName);
    FiffRawData raw;
    try {
        raw = FiffRawData(file);
    } catch (const std::exception& error) {
        qCritical("Cannot read FIFF file %s: %s", qPrintable(fifName), error.what());
        return 1;
    }
    if (raw.info.isEmpty()) {
        qCritical("Cannot read FIFF file: %s", qPrintable(fifName));
        return 1;
    }

    // Update bad channels
    qInfo("Previous bad channels: %lld" , static_cast<long long>(raw.info.bads.size()));
    raw.info.bads = bads;
    qInfo("New bad channels: %lld" , static_cast<long long>(raw.info.bads.size()));

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

    qInfo("Successfully updated bad channels in %s" , qPrintable(fifName));

    return 0;
}
