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
 * @brief    Fix magnetometer coil types in FIFF files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_ch_info.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static int processFile(const QString &name, bool doMagnes)
{
    QFile file(name);
    FiffStream::SPtr stream = FiffStream::open_update(file);
    if (!stream) {
        qCritical("Cannot open file for update: %s", qPrintable(name));
        return -1;
    }

    int found = 0;
    int nMagnes = 0;
    int nCoilNone = 0;

    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k]->kind == FIFF_CH_INFO) {
            FiffTag::UPtr tag;
            stream->read_tag(tag, stream->dir()[k]->pos);
            FiffChInfo ch = tag->toChInfo();

            bool modified = false;

            // Fix Vectorview magnetometer coil type
            if (ch.chpos.coil_type == FIFFV_COIL_VV_MAG_T1) {
                ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
                found++;
                modified = true;
            }

            // Set COIL_NONE for non-MEG/EEG channels
            if (ch.kind != FIFFV_MEG_CH &&
                ch.kind != FIFFV_EEG_CH &&
                ch.kind != FIFFV_MCG_CH &&
                ch.kind != FIFFV_REF_MEG_CH) {
                if (ch.chpos.coil_type != FIFFV_COIL_NONE) {
                    ch.chpos.coil_type = FIFFV_COIL_NONE;
                    nCoilNone++;
                    modified = true;
                }
            }

            // Fix Magnes coil types
            if (doMagnes) {
                if (ch.chpos.coil_type == FIFFV_COIL_MAGNES_MAG) {
                    ch.chpos.coil_type = FIFFV_COIL_MAGNES_GRAD;
                    found++;
                    nMagnes++;
                    modified = true;
                }
            }

            if (modified) {
                stream->device()->seek(stream->dir()[k]->pos);
                stream->write_ch_info(ch);
            }
        }
    }

    if (found)
        fprintf(stderr, "fixed %d magnetometer coil types ", found);
    else
        fprintf(stderr, "no magnetometer coil type fixes ");
    if (nCoilNone)
        fprintf(stderr, "/ fixed %d coil_type fields in non MEG/EEG channels ", nCoilNone);
    if (doMagnes)
        fprintf(stderr, "/ %d Magnes fixes ", nMagnes);

    stream->close();
    return 0;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_fix_mag_coil_types");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Fix magnetometer coil types in FIFF files.\n\n"
        "Replaces FIFFV_COIL_VV_MAG_T1 with FIFFV_COIL_VV_MAG_T3.\n"
        "Sets FIFFV_COIL_NONE for non-MEG/EEG channels."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption magnesOpt("magnes",
        "Also fix Magnes WH magnetometer coil types.");
    parser.addOption(magnesOpt);

    parser.addPositionalArgument("files", "FIFF file(s) to process.", "<file> ...");

    parser.process(app);

    bool doMagnes = parser.isSet(magnesOpt);
    QStringList files = parser.positionalArguments();

    if (files.isEmpty()) {
        qCritical("No input files specified.");
        parser.showHelp(1);
    }

    for (const QString &fname : files) {
        fprintf(stderr, "%s ... ", qPrintable(fname));
        if (processFile(fname, doMagnes) != 0) {
            fprintf(stderr, "[failed]\n");
        } else {
            fprintf(stderr, "[ok]\n");
        }
    }

    return 0;
}
