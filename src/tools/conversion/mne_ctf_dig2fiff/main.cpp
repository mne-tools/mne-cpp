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
 * @brief    Convert CTF Polhemus digitization data to FIFF and hpts formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_coord_trans.h>

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
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

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

struct DigPoint {
    int kind;       // FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA, FIFFV_POINT_EEG
    int ident;
    float r[3];     // position in meters
};

//=============================================================================================================

/**
 * Read CTF digitization text file.
 * Format: one point per line with x y z coordinates in cm.
 * First three points are fiducials (nasion, left ear, right ear).
 */
static QList<DigPoint> readCtfDig(const QString &filename, bool numFids)
{
    QList<DigPoint> points;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open CTF digitization file: %s", qPrintable(filename));
        return points;
    }

    QTextStream in(&file);
    int lineNo = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 3)
            continue;

        bool ok1, ok2, ok3;
        float x = parts[0].toFloat(&ok1);
        float y = parts[1].toFloat(&ok2);
        float z = parts[2].toFloat(&ok3);
        if (!ok1 || !ok2 || !ok3)
            continue;

        DigPoint p;
        // Convert from cm to meters
        p.r[0] = x / 100.0f;
        p.r[1] = y / 100.0f;
        p.r[2] = z / 100.0f;

        if (lineNo < 3) {
            // First three points are fiducials
            p.kind = FIFFV_POINT_CARDINAL;
            if (numFids) {
                // 1=nasion, 2=left auricular, 3=right auricular
                p.ident = lineNo + 1;
            } else {
                // Standard order: nasion, left ear, right ear
                if (lineNo == 0) p.ident = FIFFV_POINT_NASION;
                else if (lineNo == 1) p.ident = FIFFV_POINT_LPA;
                else p.ident = FIFFV_POINT_RPA;
            }
        } else {
            p.kind = FIFFV_POINT_EXTRA;
            p.ident = lineNo - 3 + 1;
        }

        points.append(p);
        lineNo++;
    }

    return points;
}

//=============================================================================================================

/**
 * Compute head coordinate transform from fiducial points (nasion, LPA, RPA).
 * The Neuromag head coordinate system:
 *   Origin at midpoint between LPA and RPA
 *   X axis toward nasion
 *   Z axis up (y x z = right hand)
 *   Y axis toward LPA
 */
static bool computeHeadTransform(const DigPoint &nasion, const DigPoint &lpa, const DigPoint &rpa,
                                  Matrix4f &trans)
{
    Vector3f n(nasion.r[0], nasion.r[1], nasion.r[2]);
    Vector3f l(lpa.r[0], lpa.r[1], lpa.r[2]);
    Vector3f r(rpa.r[0], rpa.r[1], rpa.r[2]);

    Vector3f origin = 0.5f * (l + r);
    Vector3f ex = (n - origin).normalized();
    Vector3f ez_tmp = (r - l).normalized();
    Vector3f ey = ez_tmp.cross(ex).normalized();
    Vector3f ez = ex.cross(ey).normalized();

    trans = Matrix4f::Identity();
    trans(0, 0) = ex(0); trans(0, 1) = ex(1); trans(0, 2) = ex(2);
    trans(1, 0) = ey(0); trans(1, 1) = ey(1); trans(1, 2) = ey(2);
    trans(2, 0) = ez(0); trans(2, 1) = ez(1); trans(2, 2) = ez(2);
    trans(0, 3) = -ex.dot(origin);
    trans(1, 3) = -ey.dot(origin);
    trans(2, 3) = -ez.dot(origin);

    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_ctf_dig2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert CTF Polhemus digitization data to FIFF and hpts formats.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption ctfOpt("ctf", "CTF digitization text file.", "name");
    parser.addOption(ctfOpt);

    QCommandLineOption fifOpt("fif", "Output FIFF file.", "name");
    parser.addOption(fifOpt);

    QCommandLineOption hptsOpt("hpts", "Output hpts file.", "name");
    parser.addOption(hptsOpt);

    QCommandLineOption numfidsOpt("numfids", "Fiducials are numbered (1=nasion, 2=left, 3=right).");
    parser.addOption(numfidsOpt);

    parser.process(app);

    QString ctfName = parser.value(ctfOpt);
    QString fifName = parser.value(fifOpt);
    QString hptsName = parser.value(hptsOpt);
    bool numFids = parser.isSet(numfidsOpt);

    if (ctfName.isEmpty()) {
        qCritical("--ctf is required.");
        parser.showHelp(1);
    }
    if (fifName.isEmpty() && hptsName.isEmpty()) {
        qCritical("At least one of --fif or --hpts must be specified.");
        parser.showHelp(1);
    }

    QList<DigPoint> points = readCtfDig(ctfName, numFids);
    if (points.size() < 3) {
        qCritical("Need at least 3 fiducial points, got %d", points.size());
        return 1;
    }

    fprintf(stderr, "Read %d digitization points from %s\n", points.size(), qPrintable(ctfName));

    // Compute head coordinate transform from fiducials
    Matrix4f headTrans;
    computeHeadTransform(points[0], points[1], points[2], headTrans);

    // Transform all points to head coordinates
    for (DigPoint &p : points) {
        Vector3f r(p.r[0], p.r[1], p.r[2]);
        Vector3f rt = headTrans.block<3, 3>(0, 0) * r + headTrans.block<3, 1>(0, 3);
        p.r[0] = rt(0);
        p.r[1] = rt(1);
        p.r[2] = rt(2);
    }

    // Write FIFF output
    if (!fifName.isEmpty()) {
        QFile outFile(fifName);
        FiffStream::SPtr outStream = FiffStream::start_file(outFile);
        if (!outStream) {
            qCritical("Cannot open output file: %s", qPrintable(fifName));
            return 1;
        }

        outStream->start_block(FIFFB_ISOTRAK);
        for (const DigPoint &p : points) {
            FiffDigPoint dp;
            dp.kind = p.kind;
            dp.ident = p.ident;
            dp.r[0] = p.r[0];
            dp.r[1] = p.r[1];
            dp.r[2] = p.r[2];
            dp.coord_frame = FIFFV_COORD_HEAD;
            outStream->write_dig_point(dp);
        }
        outStream->end_block(FIFFB_ISOTRAK);
        outStream->end_file();
        fprintf(stderr, "Wrote %d points to %s\n", points.size(), qPrintable(fifName));
    }

    // Write hpts output
    if (!hptsName.isEmpty()) {
        QFile hptsFile(hptsName);
        if (!hptsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open hpts file: %s", qPrintable(hptsName));
            return 1;
        }
        QTextStream out(&hptsFile);
        for (const DigPoint &p : points) {
            QString cat;
            if (p.kind == FIFFV_POINT_CARDINAL) cat = "cardinal";
            else if (p.kind == FIFFV_POINT_HPI) cat = "hpi";
            else if (p.kind == FIFFV_POINT_EEG) cat = "eeg";
            else cat = "extra";

            out << cat << " " << p.ident << " "
                << QString::number(1000.0f * p.r[0], 'f', 1) << " "
                << QString::number(1000.0f * p.r[1], 'f', 1) << " "
                << QString::number(1000.0f * p.r[2], 'f', 1) << "\n";
        }
        hptsFile.close();
        fprintf(stderr, "Wrote %d points to %s\n", points.size(), qPrintable(hptsName));
    }

    return 0;
}
