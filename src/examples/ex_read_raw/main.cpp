//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Read a segment of a raw FIFF recording with channel picking, SSP projectors and CTF compensation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/mne/mne.h>
#include <mne/base/log.h>
#include <mne/viz/backends/qt/plots/helpers/figureexporter.h>
#include <mne/viz/backends/qt/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;
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
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Raw Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QCommandLineOption fromOption("from", "Read data from <from> (in seconds).", "from", "42.956");
    QCommandLineOption toOption("to", "Read data from <to> (in seconds).", "to", "52.956");
    QCommandLineOption inSamplesOption("inSamples", "Timing is set in samples.", "inSamples", "false");
    QCommandLineOption keepCompOption("keepComp", "Keep compensators.", "keepComp", "false");

    parser.addOption(inputOption);
    parser.addOption(fromOption);
    parser.addOption(toOption);
    parser.addOption(inSamplesOption);
    parser.addOption(keepCompOption);

    parser.process(app);


    float from = parser.value(fromOption).toFloat();
    float to = parser.value(toOption).toFloat();

    bool in_samples = false;
    if (parser.value(inSamplesOption) == "false" || parser.value(inSamplesOption) == "0") {
        in_samples = false;
    } else if (parser.value(inSamplesOption) == "true" || parser.value(inSamplesOption) == "1") {
        in_samples = true;
    }

    bool keep_comp = false;
    if (parser.value(keepCompOption) == "false" || parser.value(keepCompOption) == "0") {
        keep_comp = false;
    } else if (parser.value(keepCompOption) == "true" || parser.value(keepCompOption) == "1") {
        keep_comp = true;
    }

    //! [fiff_raw_data_open]
    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(parser.value(inputOption).toStdString());

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    StringList include{"STI 014"};
    bool want_meg = true;
    bool want_eeg = false;
    bool want_stim = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
    //! [fiff_raw_data_open]

    // SSP operator and CTF compensation, both applied by read_raw_segment
    raw.activate_projections();
    if (!keep_comp) {
        if (auto compensated = raw.apply_gradient_compensation(0); !compensated) {
            printf("%s\n", compensated.error().what());
            return 1;
        }
    }
    //! [fiff_raw_data_read_segment]
    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;
    MatrixXd data;
    MatrixXd times;
    if (in_samples)
        readSuccessful = raw.read_raw_segment(data, times, (qint32)from, (qint32)to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(data, times, from, to, picks);

    if (!readSuccessful) {
        Log::warning("Could not read raw segment.\n");
        return -1;
    }
    //! [fiff_raw_data_read_segment]

    Log::info("Read {} samples.\n", (qint32)data.cols());

    std::cout << data.block(0, 0, 10, 10) << std::endl;

    // Six gradiometers in fT/cm, stacked six standard deviations apart
    QVector<int> grads;
    for (int c = 0; c < data.rows() && grads.size() < 6; ++c) {
        if (raw.info.chs[picks(c)].unit == FIFF_UNIT_T_M) {
            grads.append(c);
        }
    }
    double spread = 0.0;
    for (int c : grads) {
        const RowVectorXd row = 1e13 * data.row(c);
        spread = std::max(spread, std::sqrt((row.array() - row.mean()).square().mean()));
    }
    LinePlot traces;
    traces.setTitle(QString("Raw MEG from %1 s to %2 s").arg(from).arg(to));
    traces.setXLabel("Time [s]");
    traces.setYLabel("Gradiometers [fT/cm], offset");
    const std::vector<double> seconds(times.data(), times.data() + times.size());
    for (int i = 0; i < grads.size(); ++i) {
        const int c = grads[i];
        const double mean = 1e13 * data.row(c).mean();
        std::vector<double> values(data.cols());
        for (int s = 0; s < data.cols(); ++s) {
            values[s] = 1e13 * data(c, s) - mean - 6.0 * spread * i;
        }
        traces.addSeries(seconds, values, QString::fromStdString(raw.info.ch_names[picks(c)]));
    }
    FigureExporter::save(traces, "raw", "The first six gradiometers of the segment with the SSP projectors applied.", QSize(800, 520));

    return 0;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
