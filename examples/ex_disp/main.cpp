//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 *
 * @brief    Implements the main() application function.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/plots/imagesc.h>
#include <disp/plots/plot.h>
#include <disp/plots/tfplot.h>

#include <fiff/fiff.h>

#include <mne/mne.h>

#include <math.h>

#include <utils/spectrogram.h>
#include <utils/generics/applicationlogger.h>

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
int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Disp Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
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
    if(parser.value(inSamplesOption) == "false" || parser.value(inSamplesOption) == "0") {
        in_samples = false;
    } else if(parser.value(inSamplesOption) == "true" || parser.value(inSamplesOption) == "1") {
        in_samples = true;
    }

    bool keep_comp = false;
    if(parser.value(keepCompOption) == "false" || parser.value(keepCompOption) == "0") {
        keep_comp = false;
    } else if(parser.value(keepCompOption) == "true" || parser.value(keepCompOption) == "1") {
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
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
        printf("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
            raw.info.projs[k].active = true;

        printf("%d projection items activated\n",raw.info.projs.size());
        //
        //   Create the projector
        //
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)
            printf("The projection vectors do not apply to these channels\n");
        else
            printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
        printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
        qDebug() << "This part needs to be debugged";
        if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
        {
            raw.info.set_current_comp(dest_comp);
            printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
        }
        else
        {
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

    if (!readSuccessful)
    {
        printf("Could not read raw segment.\n");
        return -1;
    }

    printf("Read %d samples.\n",(qint32)data.cols());

    //ImageSc example
    ImageSc imagesc(data);
    imagesc.setTitle("Data ImageSc");
    imagesc.setXLabel("X Axes");
    imagesc.setYLabel("Y Axes");
    imagesc.setColorMap("Hot");//imagesc.setColorMap("Jet");//imagesc.setColorMap("RedBlue");//imagesc.setColorMap("Bone");//imagesc.setColorMap("Jet");//imagesc.setColorMap("Hot");
    imagesc.show();

    //Plot example
    VectorXd dataCol = data.col(0);
    Plot plot(dataCol);
    plot.setTitle("Data Row 0 Plot");
    plot.setXLabel("X Axes");
    plot.setYLabel("Y Axes");
    plot.setWindowTitle("Corresponding function to MATLABs plot");
    plot.show();

    //ToDo: Debug tfplot
    //tf plot example
    dataCol = data.row(0).transpose();
    MatrixXd dataSpectrum = Spectrogram::makeSpectrogram(dataCol, raw.info.sfreq*0.2);

    TFplot tfplot(dataSpectrum, raw.info.sfreq, 0, 100, ColorMaps::Jet);
    tfplot.show();

    return a.exec();
}
