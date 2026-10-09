//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     April, 2019
 * @brief    Converts EDF/EDF+ and BDF recordings to FIFF via BIDSLIB::EDFReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>

#include <bids/readers/bids_edf_reader.h>

#include <fiff/fiff_file.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("mne_edf2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("EDF/EDF+/BDF to FIFF conversion. Samples are stored in SI units like mne.io.read_raw_edf; "
                                     "channels sampled below the highest rate are skipped.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inputOption("fileIn", "The input EDF or BDF file. Needs to be specified.", "in");
    QCommandLineOption outputOption("fileOut", "The output file. If not specified, the input file name with a .fif extension.", "out");
    parser.addOption(inputOption);
    parser.addOption(outputOption);
    parser.process(a);

    const QString sInputFile = parser.value(inputOption);
    QString sOutputFile = parser.value(outputOption);
    if (sInputFile.isEmpty()) {
        parser.showHelp(1);
    }

    EDFReader reader;
    if (!reader.supportsExtension(sInputFile.mid(sInputFile.lastIndexOf('.')))) {
        qCritical() << "Not an EDF or BDF file:" << sInputFile;
        return 1;
    }
    if (!reader.open(sInputFile) || reader.getSampleCount() <= 0) {
        qCritical() << "Could not read" << sInputFile;
        return 1;
    }
    if (sOutputFile.isEmpty()) {
        sOutputFile = sInputFile.left(sInputFile.lastIndexOf('.')) + ".fif";
    }

    const FiffRawData fiffRaw = reader.toFiffRawData();
    QFile fileOut(sOutputFile);
    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(fileOut, fiffRaw.info, cals);
    if (!outfid) {
        qCritical() << "Could not write" << sOutputFile;
        return 1;
    }
    fiff_int_t first = 0;
    outfid->write_int(FIFF_FIRST_SAMPLE, &first);

    // 10 s buffers
    const int iBufferSamples = static_cast<int>(std::ceil(10.0f * fiffRaw.info.sfreq));
    const int iSampleCount = static_cast<int>(reader.getSampleCount());
    for (int iStart = 0; iStart < iSampleCount; iStart += iBufferSamples) {
        const int iEnd = std::min(iStart + iBufferSamples, iSampleCount);
        outfid->write_raw_buffer(reader.readRawSegment(iStart, iEnd).cast<double>(), cals);
    }
    outfid->finish_writing_raw();

    qInfo() << "Wrote" << sOutputFile;
    return 0;
}
