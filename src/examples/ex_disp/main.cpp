//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Disp library widgets: image map, line plot and time-frequency plot of raw data
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/plots/helpers/figureexporter.h>
#include <disp/plots/imagesc.h>
#include <disp/plots/plot.h>
#include <disp/plots/tfplot.h>

#include <fiff/fiff.h>

#include <mne/mne.h>

#include <math.h>

#include <dsp/spectrogram.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QImage>
#include <QGraphicsView>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;

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
    qInstallMessageHandler(MNELogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Disp Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption fromOption("from", "Read data from <from> (in seconds).", "from", "42.956");
    QCommandLineOption toOption("to", "Read data from <to> (in seconds).", "to", "44.670");
    QCommandLineOption inSamplesOption("inSamples", "Timing is set in samples.", "inSamples", "false");
    QCommandLineOption keepCompOption("keepComp", "Keep compensators.", "keepComp", "false");

    parser.addOption(inputOption);
    parser.addOption(fromOption);
    parser.addOption(toOption);
    parser.addOption(inSamplesOption);
    parser.addOption(keepCompOption);

    parser.process(a);

    QFile t_fileRaw(parser.value(inputOption));

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

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg = true;
    bool want_eeg = false;
    bool want_stim = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
        printf("No projector specified for these data\n");
    else {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
            raw.info.projs[k].active = true;

        printf("%lld projection items activated\n",
               static_cast<long long>(raw.info.projs.size()));
        //
        //   Create the projector
        //
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)
            printf("The projection vectors do not apply to these channels\n");
        else
            printf("Created an SSP operator (subspace dimension = %d)\n", nproj);
    }

    //
    //   Set up the compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
        printf("Current compensation grade : %d\n", current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp) {
        qDebug() << "This part needs to be debugged";
        if (MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp)) {
            raw.info.set_current_comp(dest_comp);
            printf("Appropriate compensator added to change to grade %d.\n", dest_comp);
        } else {
            printf("Could not make the compensator\n");
            //return -1;
        }
    }

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
        printf("Could not read raw segment.\n");
        return -1;
    }

    printf("Read %d samples.\n", (qint32)data.cols());

    //ImageSc example: one row per channel, one column per sample
    ImageSc imagesc(data);
    imagesc.setTitle("MEG channels over time");
    imagesc.setXLabel("Sample");
    imagesc.setYLabel("Channel");
    imagesc.setColorMap("Hot"); // also "Jet", "RedBlue", "Bone", "Viridis"

    //Plot example: the first channel over time
    VectorXd dataCol = data.row(0).transpose();
    Plot plot(dataCol);
    plot.setTitle(raw.info.ch_names[picks(0)]);
    plot.setXLabel("Sample");
    plot.setYLabel("Amplitude");
    plot.setWindowTitle("Corresponding function to MATLABs plot");

    //tf plot example: spectrogram of the same channel
    MatrixXd dataSpectrum = Spectrogram::makeSpectrogram(dataCol, raw.info.sfreq * 0.2);
    TFplot tfplot(dataSpectrum, raw.info.sfreq, 0, 100, ColorMaps::Jet, times(0, 0));

    FigureExporter::save(imagesc, "image_sc", QString("ImageSc of %1 MEG channels from %2 s to %3 s.").arg(data.rows()).arg(from).arg(to));
    FigureExporter::save(plot, "plot", QString("Plot of %1 over the same segment.").arg(raw.info.ch_names[picks(0)]));
    FigureExporter::save(tfplot, "tf_plot", QString("TFplot: spectrogram (0-100 Hz) of %1.").arg(raw.info.ch_names[picks(0)]));
    if (!FigureExporter::directory().isEmpty()) {
        return 0;
    }
    imagesc.show();
    plot.show();
    tfplot.show();

    return a.exec();
}
