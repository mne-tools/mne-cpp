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
 * @brief    Convert FreeSurfer annotation files to individual label files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_annotation.h>
#include <fs/fs_surface.h>
#include <fs/fs_label.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Convert FreeSurfer annotation file to individual label files.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --subject <name>     Subject name\n");
    fprintf(stderr, "  --subjects_dir <dir> Subjects directory (default: $SUBJECTS_DIR)\n");
    fprintf(stderr, "  --annot <name>       Annotation name (e.g., aparc, aparc.a2009s)\n");
    fprintf(stderr, "  --hemi <hemi>        Hemisphere: lh or rh (default: both)\n");
    fprintf(stderr, "  --outdir <dir>       Output directory for label files (default: subject's label/)\n");
    fprintf(stderr, "  --surf <name>        Surface to use for coordinates (default: white)\n");
    fprintf(stderr, "  --help               Print this help\n");
    fprintf(stderr, "  --version            Print version\n");
}

//=============================================================================================================

static bool writeLabel(const QString &filename, const QString &comment,
                       const VectorXi &vertices, const MatrixX3f &positions,
                       const VectorXd &values)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot write label file: %s", qPrintable(filename));
        return false;
    }
    QTextStream out(&file);
    out << "#" << comment << "\n";
    out << vertices.size() << "\n";
    for (int i = 0; i < vertices.size(); i++) {
        out << QString::asprintf("%d  %8.4f  %8.4f  %8.4f  %10.6f\n",
            vertices(i),
            positions(i, 0), positions(i, 1), positions(i, 2),
            values(i));
    }
    return true;
}

//=============================================================================================================

static bool processHemisphere(const QString &subjectsDir, const QString &subject,
                              const QString &hemi, const QString &annotName,
                              const QString &surfName, const QString &outDir)
{
    // Read surface for vertex coordinates
    QString surfPath = QString("%1/%2/surf/%3.%4").arg(subjectsDir, subject, hemi, surfName);
    FsSurface surface;
    if (!FsSurface::read(surfPath, surface)) {
        qCritical("Cannot read surface: %s", qPrintable(surfPath));
        return false;
    }
    printf("Read surface: %s (%d vertices)\n", qPrintable(surfPath), surface.rr().rows());

    // Read annotation
    QString annotPath = QString("%1/%2/label/%3.%4.annot").arg(subjectsDir, subject, hemi, annotName);
    FsAnnotation annot;
    if (!FsAnnotation::read(annotPath, annot)) {
        qCritical("Cannot read annotation: %s", qPrintable(annotPath));
        return false;
    }
    printf("Read annotation: %s\n", qPrintable(annotPath));

    // Convert to labels using the library method
    QList<FsLabel> labels;
    QList<Eigen::RowVector4i> labelRgbas;
    annot.toLabels(surface, labels, labelRgbas);

    printf("Found %d labels in %s hemisphere\n", labels.size(), qPrintable(hemi));

    // Write each label to a file
    int written = 0;
    for (int i = 0; i < labels.size(); i++) {
        const FsLabel &label = labels[i];
        QString labelName = label.name;
        if (labelName.isEmpty())
            labelName = QString("label_%1").arg(i);

        QString filename = QString("%1/%2-%3.label").arg(outDir, labelName, hemi);

        VectorXi verts = label.vertices;
        MatrixX3f pos(verts.size(), 3);
        VectorXd values = VectorXd::Zero(verts.size());

        for (int v = 0; v < verts.size(); v++) {
            int vi = verts(v);
            if (vi < surface.rr().rows()) {
                pos(v, 0) = surface.rr()(vi, 0);
                pos(v, 1) = surface.rr()(vi, 1);
                pos(v, 2) = surface.rr()(vi, 2);
            }
        }

        QString comment = QString(" %1 annotation %2 %3").arg(annotName, hemi, labelName);
        if (writeLabel(filename, comment, verts, pos, values)) {
            written++;
        }
    }

    printf("Wrote %d label files for %s hemisphere\n", written, qPrintable(hemi));
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString subject;
    QString subjectsDir = qEnvironmentVariable("SUBJECTS_DIR");
    QString annotName = "aparc";
    QString surfName = "white";
    QString outDir;
    QString hemi;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        }
        else if (strcmp(argv[k], "--subject") == 0) {
            if (++k >= argc) { qCritical("--subject: argument required."); return 1; }
            subject = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--subjects_dir") == 0) {
            if (++k >= argc) { qCritical("--subjects_dir: argument required."); return 1; }
            subjectsDir = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--annot") == 0) {
            if (++k >= argc) { qCritical("--annot: argument required."); return 1; }
            annotName = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--hemi") == 0) {
            if (++k >= argc) { qCritical("--hemi: argument required."); return 1; }
            hemi = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--outdir") == 0) {
            if (++k >= argc) { qCritical("--outdir: argument required."); return 1; }
            outDir = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--surf") == 0) {
            if (++k >= argc) { qCritical("--surf: argument required."); return 1; }
            surfName = QString(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (subject.isEmpty()) { qCritical("--subject is required."); usage(argv[0]); return 1; }
    if (subjectsDir.isEmpty()) { qCritical("--subjects_dir or $SUBJECTS_DIR is required."); return 1; }

    // Default output dir
    if (outDir.isEmpty())
        outDir = QString("%1/%2/label").arg(subjectsDir, subject);

    QDir().mkpath(outDir);

    bool ok = true;
    if (hemi.isEmpty() || hemi == "lh")
        ok = processHemisphere(subjectsDir, subject, "lh", annotName, surfName, outDir) && ok;
    if (hemi.isEmpty() || hemi == "rh")
        ok = processHemisphere(subjectsDir, subject, "rh", annotName, surfName, outDir) && ok;

    return ok ? 0 : 1;
}
