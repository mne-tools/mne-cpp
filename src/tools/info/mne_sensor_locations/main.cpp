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
 * @brief    Export sensor locations from a measurement FIFF file to text.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_node.h>

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

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_sensor_locations");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Export sensor locations to text.\n\n"
                                     "Reads channel locations from a measurement FIFF file and writes\n"
                                     "channel_name x y z (one per line) for MEG and EEG channels.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Measurement FIFF file.", "file");
    parser.addOption(measOpt);

    QCommandLineOption outOpt("out", "Output text file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption frameOpt("frame", "Coordinate frame: \"head\" (default) or \"device\".", "frame", "head");
    parser.addOption(frameOpt);

    parser.process(app);

    QString measFile = parser.value(measOpt);
    QString outFile = parser.value(outOpt);
    QString frame = parser.value(frameOpt).toLower();

    if (measFile.isEmpty() || outFile.isEmpty()) {
        qCritical("Both --meas and --out are required.");
        parser.showHelp(1);
    }
    if (frame != "head" && frame != "device") {
        qCritical("--frame must be \"head\" or \"device\".");
        return 1;
    }

    // Read measurement info
    QFile fMeas(measFile);
    FiffStream::SPtr stream(new FiffStream(&fMeas));
    if (!stream->open()) {
        qCritical("Cannot open FIFF file: %s", qPrintable(measFile));
        return 1;
    }

    FiffInfo info;
    FiffDirNode::SPtr infoNode;
    if (!stream->read_meas_info(stream->dirtree(), info, infoNode)) {
        qCritical("Cannot read measurement info from: %s", qPrintable(measFile));
        return 1;
    }
    stream->close();

    // Prepare coordinate transform if device frame requested
    // Channel positions for MEG are in device coords by default;
    // EEG positions are in head coords by default.
    // dev_head_t: device -> head transform
    bool useDevice = (frame == "device");
    FiffCoordTrans headToDev;
    if (useDevice) {
        headToDev = info.dev_head_t.inverted();
    }

    // Open output file
    FILE *out = fopen(qPrintable(outFile), "w");
    if (!out) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    int nWritten = 0;
    for (int k = 0; k < info.chs.size(); ++k) {
        const FiffChInfo &ch = info.chs[k];

        float x, y, z;

        if (ch.kind == FIFFV_MEG_CH || ch.kind == FIFFV_REF_MEG_CH) {
            // MEG coil location (device coords)
            x = ch.chpos.r0[0];
            y = ch.chpos.r0[1];
            z = ch.chpos.r0[2];

            if (!useDevice) {
                // Transform device -> head
                Vector3f pos(x, y, z);
                MatrixX3f posMat(1, 3);
                posMat.row(0) = pos.transpose();
                MatrixX3f result = info.dev_head_t.apply_trans(posMat);
                x = result(0, 0);
                y = result(0, 1);
                z = result(0, 2);
            }
        } else if (ch.kind == FIFFV_EEG_CH) {
            // EEG electrode location (head coords)
            x = ch.eeg_loc(0, 0);
            y = ch.eeg_loc(1, 0);
            z = ch.eeg_loc(2, 0);

            if (useDevice) {
                // Transform head -> device
                Vector3f pos(x, y, z);
                MatrixX3f posMat(1, 3);
                posMat.row(0) = pos.transpose();
                MatrixX3f result = headToDev.apply_trans(posMat);
                x = result(0, 0);
                y = result(0, 1);
                z = result(0, 2);
            }
        } else {
            continue;
        }

        fprintf(out, "%-20s %12.6f %12.6f %12.6f\n",
                qPrintable(ch.ch_name), x, y, z);
        ++nWritten;
    }

    fclose(out);
    fprintf(stderr, "Wrote %d sensor locations to %s (frame: %s)\n",
            nWritten, qPrintable(outFile), qPrintable(frame));

    return 0;
}
