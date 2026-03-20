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
 * @brief    Collect coordinate transformations into one FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_node.h>

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

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void printTransform(const FiffCoordTrans &t)
{
    printf("%s -> %s transform:\n",
           qPrintable(FiffCoordTrans::frame_name(t.from)),
           qPrintable(FiffCoordTrans::frame_name(t.to)));
    for (int i = 0; i < 3; i++) {
        printf("  %10.6f %10.6f %10.6f  %10.4f mm\n",
               t.trans(i, 0), t.trans(i, 1), t.trans(i, 2),
               1000.0f * t.trans(i, 3));
    }
    printf("\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_collect_transforms");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Collect coordinate transforms from FIFF files.\n\nReads device-to-head and MRI-to-head transforms.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "MEG measurement file (device->head transform).", "name");
    parser.addOption(measOpt);

    QCommandLineOption mriOpt("mri", "FIFF MRI description file (MRI->head transform).", "name");
    parser.addOption(mriOpt);

    QCommandLineOption outOpt("out", "Output file name.", "name");
    parser.addOption(outOpt);

    parser.process(app);

    QString measName = parser.value(measOpt);
    QString mriName = parser.value(mriOpt);
    QString outName = parser.value(outOpt);

    if (measName.isEmpty() && mriName.isEmpty()) {
        qCritical("At least one of --meas or --mri must be specified.");
        parser.showHelp(1);
    }

    if (!measName.isEmpty())
        fprintf(stderr, "MEG/EEG data file      : %s\n", qPrintable(measName));
    if (!mriName.isEmpty())
        fprintf(stderr, "MEG/MRI transform file : %s\n", qPrintable(mriName));
    fprintf(stderr, "\n");

    // Collect transforms
    FiffCoordTrans devHeadT;
    FiffCoordTrans mriHeadT;
    bool hasDevHead = false;
    bool hasMriHead = false;

    if (!measName.isEmpty()) {
        fprintf(stderr, "Reading %s...", qPrintable(measName));
        QFile measFile(measName);
        FiffStream::SPtr stream(new FiffStream(&measFile));
        if (!stream->open()) {
            qCritical("\nCannot open measurement file: %s", qPrintable(measName));
            return 1;
        }
        // Search for device->head transform in the file
        for (int k = 0; k < stream->nent(); k++) {
            if (stream->dir()[k]->kind == FIFF_COORD_TRANS) {
                FiffTag::SPtr tag;
                stream->read_tag(tag, stream->dir()[k]->pos);
                FiffCoordTrans t = tag->toCoordTrans();
                if (t.from == FIFFV_COORD_DEVICE && t.to == FIFFV_COORD_HEAD) {
                    devHeadT = t;
                    hasDevHead = true;
                    break;
                }
            }
        }
        stream->close();
        if (hasDevHead)
            fprintf(stderr, "[done]\n");
        else
            fprintf(stderr, "[device->head transform not found]\n");
    }

    if (!mriName.isEmpty()) {
        fprintf(stderr, "Reading %s...", qPrintable(mriName));
        QFile mriFile(mriName);
        FiffStream::SPtr stream(new FiffStream(&mriFile));
        if (!stream->open()) {
            qCritical("\nCannot open MRI file: %s", qPrintable(mriName));
            return 1;
        }
        // Search for MRI->head transform
        for (int k = 0; k < stream->nent(); k++) {
            if (stream->dir()[k]->kind == FIFF_COORD_TRANS) {
                FiffTag::SPtr tag;
                stream->read_tag(tag, stream->dir()[k]->pos);
                FiffCoordTrans t = tag->toCoordTrans();
                if (t.from == FIFFV_COORD_MRI && t.to == FIFFV_COORD_HEAD) {
                    mriHeadT = t;
                    hasMriHead = true;
                }
            }
        }
        stream->close();
        if (hasMriHead)
            fprintf(stderr, "[done]\n");
        else
            fprintf(stderr, "[MRI->head transform not found]\n");
    }

    // Print transforms
    fprintf(stderr, "\n");
    if (hasDevHead)
        printTransform(devHeadT);
    if (hasMriHead)
        printTransform(mriHeadT);

    // Write output if requested
    if (!outName.isEmpty()) {
        fprintf(stderr, "Writing %s...", qPrintable(outName));
        QFile outFile(outName);
        FiffStream::SPtr outStream = FiffStream::start_file(outFile);
        if (!outStream) {
            qCritical("\nCannot open output file: %s", qPrintable(outName));
            return 1;
        }
        if (hasDevHead)
            outStream->write_coord_trans(devHeadT);
        if (hasMriHead)
            outStream->write_coord_trans(mriHeadT);
        outStream->end_file();
        fprintf(stderr, "[done]\n");
    }

    return 0;
}
