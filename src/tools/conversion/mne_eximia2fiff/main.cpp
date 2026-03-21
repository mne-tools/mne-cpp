//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
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
 * @brief    Implements mne_eximia2fiff: convert eXimia EEG data to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================

static bool createBrainVisionFiles(const QString& nxeFile, const QString& baseName,
                                   float sfreq, int nchan)
{
    // Create .vhdr file (Brain Vision header)
    QString vhdrFile = baseName + ".vhdr";
    QString vmrkFile = baseName + ".vmrk";
    QString eegFile = nxeFile; // NXE file is the raw data

    QFile vhdr(vhdrFile);
    if (!vhdr.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Cannot create vhdr file: %s\n", vhdrFile.toUtf8().constData());
        return false;
    }
    QTextStream vhdrOut(&vhdr);
    vhdrOut << "Brain Vision Data Exchange Header File Version 1.0\n";
    vhdrOut << "; Created by mne_eximia2fiff\n\n";
    vhdrOut << "[Common Infos]\n";
    vhdrOut << "DataFile=" << QFileInfo(eegFile).fileName() << "\n";
    vhdrOut << "MarkerFile=" << QFileInfo(vmrkFile).fileName() << "\n";
    vhdrOut << "DataFormat=BINARY\n";
    vhdrOut << "DataOrientation=MULTIPLEXED\n";
    vhdrOut << "NumberOfChannels=" << nchan << "\n";
    vhdrOut << "SamplingInterval=" << static_cast<int>(1000000.0 / sfreq) << "\n\n";
    vhdrOut << "[Binary Infos]\n";
    vhdrOut << "BinaryFormat=IEEE_FLOAT_32\n\n";
    vhdrOut << "[Channel Infos]\n";
    for (int k = 1; k <= nchan; k++) {
        vhdrOut << "Ch" << k << "=EEG" << QString::number(k).rightJustified(3, '0')
                << ",,1\n";
    }
    vhdr.close();

    // Create .vmrk file (Brain Vision marker)
    QFile vmrk(vmrkFile);
    if (!vmrk.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Cannot create vmrk file: %s\n", vmrkFile.toUtf8().constData());
        return false;
    }
    QTextStream vmrkOut(&vmrk);
    vmrkOut << "Brain Vision Data Exchange Marker File, Version 1.0\n";
    vmrkOut << "; Created by mne_eximia2fiff\n\n";
    vmrkOut << "[Common Infos]\n";
    vmrkOut << "DataFile=" << QFileInfo(eegFile).fileName() << "\n\n";
    vmrkOut << "[Marker Infos]\n";
    vmrkOut << "Mk1=New Segment,,1,1,0\n";
    vmrk.close();

    printf("Created Brain Vision header: %s\n", vhdrFile.toUtf8().constData());
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_eximia2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert eXimia EEG (.nxe) data to FIFF format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption nxeOpt("nxe", "eXimia .nxe data file.", "file");
    parser.addOption(nxeOpt);
    QCommandLineOption digOpt("dig", "Digitizer data file.", "file");
    parser.addOption(digOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);
    QCommandLineOption sfreqOpt("sfreq", "Sampling frequency.", "Hz", "1450");
    parser.addOption(sfreqOpt);
    QCommandLineOption nchanOpt("nchan", "Number of channels.", "count", "64");
    parser.addOption(nchanOpt);

    parser.process(app);

    QString nxeFile = parser.value(nxeOpt);
    QString digFile = parser.value(digOpt);
    QString outFile = parser.value(outOpt);
    float sfreq = parser.value(sfreqOpt).toFloat();
    int nchan = parser.value(nchanOpt).toInt();

    if (nxeFile.isEmpty()) {
        fprintf(stderr, "eXimia .nxe file (--nxe) is required.\n");
        return 1;
    }
    if (outFile.isEmpty()) {
        fprintf(stderr, "Output file (--out) is required.\n");
        return 1;
    }

    //
    // Step 1: Convert digitizer data to FIFF (if provided)
    //
    QString digFifFile;
    if (!digFile.isEmpty()) {
        digFifFile = QFileInfo(outFile).dir().filePath("dig_temp.fif");

        // Convert hpts/text digitizer to FIFF
        QProcess digProc;
        QStringList digArgs;
        digArgs << "--hpts" << digFile << "--fifout" << digFifFile;

        printf("Converting digitizer data...\n");
        digProc.start("mne_convert_dig_data", digArgs);
        if (!digProc.waitForFinished(30000) || digProc.exitCode() != 0) {
            fprintf(stderr, "Warning: Failed to convert digitizer data. Proceeding without.\n");
            digFifFile.clear();
        }
    }

    //
    // Step 2: Create Brain Vision format intermediary files
    //
    QString baseName = QFileInfo(outFile).dir().filePath(
        QFileInfo(nxeFile).completeBaseName());

    if (!createBrainVisionFiles(nxeFile, baseName, sfreq, nchan))
        return 1;

    //
    // Step 3: Convert Brain Vision to FIFF
    //
    QProcess bvProc;
    QStringList bvArgs;
    bvArgs << "--vhdr" << (baseName + ".vhdr");
    bvArgs << "--out" << outFile;
    bvArgs << "--eximia"; // Special eXimia mode

    if (!digFifFile.isEmpty()) {
        bvArgs << "--dig" << digFifFile;
    }

    printf("Converting to FIFF...\n");
    bvProc.start("mne_brain_vision2fiff", bvArgs);
    if (!bvProc.waitForFinished(60000)) {
        fprintf(stderr, "mne_brain_vision2fiff timed out.\n");
        return 1;
    }

    if (bvProc.exitCode() != 0) {
        fprintf(stderr, "mne_brain_vision2fiff failed with exit code %d\n", bvProc.exitCode());
        fprintf(stderr, "%s\n", bvProc.readAllStandardError().constData());
        return 1;
    }

    // Cleanup temp files
    QFile::remove(baseName + ".vhdr");
    QFile::remove(baseName + ".vmrk");
    if (!digFifFile.isEmpty())
        QFile::remove(digFifFile);

    printf("Successfully converted eXimia data to: %s\n", outFile.toUtf8().constData());
    return 0;
}
