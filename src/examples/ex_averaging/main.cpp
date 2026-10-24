//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.3
 * @date     July, 2020
 * @brief    Average stimulus-locked epochs of a raw FIFF recording into an evoked response, with event selection and EOG rejection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>

#include <mne/mne.h>
#include <mne/mne_epoch_data_list.h>

#include <utils/generics/mne_logger.h>

#include <disp/plots/helpers/figureexporter.h>
#include <disp/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
#ifdef STATICBUILD

#endif

    qInstallMessageHandler(MNELogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Average Data Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption evokedIdxOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");
    QCommandLineOption pickAllOption("pickAll", "Pick all channels.", "pickAll", "true");
    QCommandLineOption keepCompOption("keepComp", "Keep compensators.", "keepComp", "false");
    QCommandLineOption destCompsOption("destComps", "<Destination> of the compensator which is to be calculated.", "destination", "0");

    parser.addOption(inputOption);
    parser.addOption(eventsFileOption);
    parser.addOption(evokedIdxOption);
    parser.addOption(pickAllOption);
    parser.addOption(keepCompOption);
    parser.addOption(destCompsOption);

    parser.process(a);

    //Load data
    QString t_fileRawName = parser.value(inputOption);
    QFile t_fileRaw(t_fileRawName);

    qint32 event = parser.value(evokedIdxOption).toInt();
    QString t_sEventName = parser.value(eventsFileOption);
    float fTMin = -0.2f;
    float fTMax = 0.5f;

    bool keep_comp = false;
    if (parser.value(keepCompOption) == "false" || parser.value(keepCompOption) == "0") {
        keep_comp = false;
    } else if (parser.value(keepCompOption) == "true" || parser.value(keepCompOption) == "1") {
        keep_comp = true;
    }

    fiff_int_t dest_comp = parser.value(destCompsOption).toInt();

    bool pick_all = false;
    if (parser.value(pickAllOption) == "false" || parser.value(pickAllOption) == "0") {
        pick_all = false;
    } else if (parser.value(pickAllOption) == "true" || parser.value(pickAllOption) == "1") {
        pick_all = true;
    }

    // Setup for reading the raw data
    FiffRawData raw(t_fileRaw);
    MNE::setup_compensators(raw, dest_comp, keep_comp);

    RowVectorXi picks;
    if (pick_all) {
        // Pick all
        picks.resize(raw.info.nchan);

        for (qint32 k = 0; k < raw.info.nchan; ++k) {
            picks(k) = k;
        }
    } else {
        QStringList include;
        include << "STI 014";
        bool want_meg = true;
        bool want_eeg = false;
        bool want_stim = false;

        picks = raw.info.pick_types(want_meg,
                                    want_eeg,
                                    want_stim,
                                    include,
                                    raw.info.bads);
    }

    // Read the events
    MatrixXi events;
    MNE::read_events(t_sEventName,
                     t_fileRawName,
                     events);

    // Read the epochs and reject epochs with EOG higher than 300e-06
    QMap<QString, double> mapReject;
    mapReject.insert("eog", 300e-06);

    FiffEvoked evoked = MNEEpochDataList::computeAverage(raw,
                                                         events,
                                                         fTMin,
                                                         fTMax,
                                                         event,
                                                         true,
                                                         fTMin,
                                                         0.0f,
                                                         mapReject,
                                                         QStringList(),
                                                         picks);

    qInfo().noquote() << QString("Averaged %1 epochs of event %2 over %3 channels and %4 samples.")
                             .arg(evoked.nave)
                             .arg(event)
                             .arg(evoked.data.rows())
                             .arg(evoked.data.cols());

    // Butterfly plot of the gradiometers in fT/cm against time in ms
    QVector<double> times(evoked.times.size());
    for (int s = 0; s < times.size(); ++s) {
        times[s] = 1000.0 * evoked.times(s);
    }
    LinePlot butterfly;
    butterfly.setTitle(QString("Evoked response to event %1 (%2 epochs)").arg(event).arg(evoked.nave));
    butterfly.setXLabel("Time [ms]");
    butterfly.setYLabel("Gradiometers [fT/cm]");
    for (int c = 0; c < evoked.data.rows(); ++c) {
        if (evoked.info.chs[c].unit == FIFF_UNIT_T_M && !evoked.info.bads.contains(evoked.info.ch_names[c])) {
            QVector<double> values(times.size());
            for (int s = 0; s < times.size(); ++s) {
                values[s] = 1e13 * evoked.data(c, s);
            }
            butterfly.addSeries(times, values, QString(), QColor(30, 60, 110, 70));
        }
    }
    FigureExporter::save(butterfly, "evoked", QString("Butterfly plot of the %1 good gradiometers averaged over %2 epochs of event %3, baseline-corrected over the prestimulus interval.").arg(butterfly.seriesCount()).arg(evoked.nave).arg(event));

    return 0;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
