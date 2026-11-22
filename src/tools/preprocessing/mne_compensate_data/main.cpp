//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Apply CTF software compensation to raw data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ctf_comp.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>

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

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_compensate_data");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Apply CTF software compensation to raw FIFF data.\n\nSets the desired compensation grade (0-3).");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input raw FIFF file.", "file");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption gradeOpt("grade", "Desired compensation grade (0, 1, 2, or 3).", "value");
    parser.addOption(gradeOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString outFile = parser.value(outOpt);
    int grade = parser.isSet(gradeOpt) ? parser.value(gradeOpt).toInt() : -1;

    if (inFile.isEmpty()) {
        Log::critical("--in is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }
    if (grade < 0) {
        Log::critical("--grade is required (0, 1, 2, or 3).");
        return 1;
    }

    // Open input file
    FiffRawData raw(inFile.toStdU16String());
    if (raw.info.empty()) {
        Log::critical("Cannot read raw data from: {}", inFile.toStdString());
        return 1;
    }

    qint32 currentComp = raw.info.get_current_comp();
    Log::info("Current compensation grade: {}", currentComp);
    Log::info("Desired compensation grade: {}", grade);

    if (currentComp == grade) {
        Log::info("Data already at desired compensation grade. No changes needed.");
        // Just copy the file
        QFile::copy(inFile, outFile);
        return 0;
    }

    // Make compensator matrix
    FiffCtfComp comp;
    if (!raw.info.make_compensator(currentComp, grade, comp)) {
        Log::critical("Cannot create compensator from grade {} to {}", currentComp, grade);
        return 1;
    }
    Log::info("Compensation matrix created ({} -> {})", currentComp, grade);

    // Read all data
    MatrixXd data;
    MatrixXd times;

    if (!raw.read_raw_segment(data, times)) {
        Log::critical("Cannot read raw data segment");
        return 1;
    }
    Log::info("Read {} channels x {} samples", static_cast<int>(data.rows()), static_cast<int>(data.cols()));

    // Apply compensation
    data = comp.data.data.cast<double>() * data;
    Log::info("Compensation applied.");

    // Update compensation info
    raw.info.set_current_comp(grade);

    // Write output
    Eigen::RowVectorXd cals;
    auto outStream = FiffStream::start_writing_raw(outFile.toStdU16String(), raw.info, cals);
    if (!outStream) {
        Log::critical("Cannot open output file: {}", outFile.toStdString());
        return 1;
    }

    // Write data in chunks
    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk);
    }

    outStream->finish_writing_raw();
    Log::info("Written compensated data to: {}", outFile.toStdString());

    return 0;
}
