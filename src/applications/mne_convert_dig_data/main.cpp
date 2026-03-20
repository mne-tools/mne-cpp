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
 * @brief    Convert Polhemus digitization data between FIFF, hpts, and elp formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Convert Polhemus digitization data between formats.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --fif <name>     Input FIFF file with digitization data\n");
    fprintf(stderr, "  --hpts <name>    Input hpts text file\n");
    fprintf(stderr, "  --fifout <name>  Output FIFF file\n");
    fprintf(stderr, "  --hptsout <name> Output hpts file\n");
    fprintf(stderr, "  --help           Print this help\n");
    fprintf(stderr, "  --version        Print version\n");
}

//=============================================================================================================

struct DigPt {
    int kind;
    int ident;
    float r[3]; // meters
};

//=============================================================================================================

static QList<DigPt> readFifDig(const QString &filename)
{
    QList<DigPt> points;
    QFile file(filename);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        qCritical("Cannot open FIFF file: %s", qPrintable(filename));
        return points;
    }

    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k]->kind == FIFF_DIG_POINT) {
            FiffTag::SPtr tag;
            stream->read_tag(tag, stream->dir()[k]->pos);
            FiffDigPoint dp = tag->toDigPoint();
            DigPt p;
            p.kind = dp.kind;
            p.ident = dp.ident;
            p.r[0] = dp.r[0];
            p.r[1] = dp.r[1];
            p.r[2] = dp.r[2];
            points.append(p);
        }
    }
    stream->close();
    return points;
}

//=============================================================================================================

static QList<DigPt> readHpts(const QString &filename)
{
    QList<DigPt> points;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open hpts file: %s", qPrintable(filename));
        return points;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 5)
            continue;

        DigPt p;
        QString cat = parts[0].toLower();
        if (cat == "cardinal" || cat == "fiducial") p.kind = FIFFV_POINT_CARDINAL;
        else if (cat == "hpi") p.kind = FIFFV_POINT_HPI;
        else if (cat == "eeg") p.kind = FIFFV_POINT_EEG;
        else p.kind = FIFFV_POINT_EXTRA;

        p.ident = parts[1].toInt();
        // hpts coordinates are in mm
        p.r[0] = parts[2].toFloat() / 1000.0f;
        p.r[1] = parts[3].toFloat() / 1000.0f;
        p.r[2] = parts[4].toFloat() / 1000.0f;

        points.append(p);
    }
    return points;
}

//=============================================================================================================

static bool writeFifDig(const QString &filename, const QList<DigPt> &points)
{
    QFile outFile(filename);
    FiffStream::SPtr outStream = FiffStream::start_file(outFile);
    if (!outStream) {
        qCritical("Cannot open output FIFF file: %s", qPrintable(filename));
        return false;
    }

    outStream->start_block(FIFFB_ISOTRAK);
    for (const DigPt &p : points) {
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

    return true;
}

//=============================================================================================================

static bool writeHpts(const QString &filename, const QList<DigPt> &points)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot open hpts output file: %s", qPrintable(filename));
        return false;
    }

    QTextStream out(&file);
    for (const DigPt &p : points) {
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

    file.close();
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString fifIn, hptsIn, fifOut, hptsOut;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        } else if (strcmp(argv[k], "--fif") == 0) {
            if (k + 1 >= argc) { qCritical("--fif: argument required."); return 1; }
            fifIn = QString(argv[++k]);
        } else if (strcmp(argv[k], "--hpts") == 0) {
            if (k + 1 >= argc) { qCritical("--hpts: argument required."); return 1; }
            hptsIn = QString(argv[++k]);
        } else if (strcmp(argv[k], "--fifout") == 0) {
            if (k + 1 >= argc) { qCritical("--fifout: argument required."); return 1; }
            fifOut = QString(argv[++k]);
        } else if (strcmp(argv[k], "--hptsout") == 0) {
            if (k + 1 >= argc) { qCritical("--hptsout: argument required."); return 1; }
            hptsOut = QString(argv[++k]);
        } else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (fifIn.isEmpty() && hptsIn.isEmpty()) {
        qCritical("Specify input with --fif or --hpts.");
        usage(argv[0]);
        return 1;
    }
    if (fifOut.isEmpty() && hptsOut.isEmpty()) {
        qCritical("Specify output with --fifout or --hptsout.");
        usage(argv[0]);
        return 1;
    }

    // Read input
    QList<DigPt> points;
    if (!fifIn.isEmpty()) {
        points = readFifDig(fifIn);
        fprintf(stderr, "Read %d points from %s\n", points.size(), qPrintable(fifIn));
    } else {
        points = readHpts(hptsIn);
        fprintf(stderr, "Read %d points from %s\n", points.size(), qPrintable(hptsIn));
    }

    if (points.isEmpty()) {
        qCritical("No digitization points read.");
        return 1;
    }

    // Write output(s)
    if (!fifOut.isEmpty()) {
        if (!writeFifDig(fifOut, points))
            return 1;
        fprintf(stderr, "Wrote %d points to %s\n", points.size(), qPrintable(fifOut));
    }
    if (!hptsOut.isEmpty()) {
        if (!writeHpts(hptsOut, points))
            return 1;
        fprintf(stderr, "Wrote %d points to %s\n", points.size(), qPrintable(hptsOut));
    }

    fprintf(stderr, "done.\n");
    return 0;
}
