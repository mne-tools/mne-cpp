//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Blerta Hamzallari <blerta.hamzallari@tu-ilmenau.de>;
 *           Felix Griesau <Felix.Griesau@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2017
 *
 * @section  LICENSE
 *
 *
 *                      Copyright (C) 2017, Lars Debor, Blerta Hamzallari, Felix Griesau, Lorenz Esch, Simon Heinke. All rights reserved.
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
 * @brief    Example of using the interpolation library and geometryInfo library
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_sourceestimate.h>
#include <mne/mne_bem.h>

#include <iostream>

#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/interpolation/interpolation.h>
#include <fiff/fiff_constants.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace DISP3DLIB;
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
int main(int argc, char *argv[])
{
    #ifdef STATICBUILD
    // Q_INIT_RESOURCE(mne_disp3d);
    #endif
    
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("ex_interpolation");
    parser.addHelpOption();

    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(sampleEvokedFileOption);

    parser.process(a);

    //acquire sensor positions
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));

    // Load data
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return 1;
    }

    // positions of EEG and MEG sensors
    QVector<Vector3f> eegSensors;
    QVector<Vector3f> megSensors;
    //fill both QVectors with the right sensor positions
    for( const FiffChInfo &info : evoked.info.chs)
    {
        //EEG
        if(info.kind == FIFFV_EEG_CH && info.unit == FIFF_UNIT_V)
        {
            eegSensors.push_back(info.chpos.r0);
        }
        //MEG
        if(info.kind == FIFFV_MEG_CH && info.unit == FIFF_UNIT_T)
        {
            megSensors.push_back(info.chpos.r0);
        }
    }

    //acquire surface data
    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

    //projecting with MEG
    qint64 startTimeProjecting = QDateTime::currentMSecsSinceEpoch();
    QVector<int> mappedSubSet = GeometryInfo::projectSensors(t_sensorSurfaceVV[0].rr, megSensors);
    std::cout <<  "Projecting duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeProjecting <<" ms " << std::endl;

    //SCDC with cancel distance 0.03
    qint64 startTimeScdc = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(t_sensorSurfaceVV[0].rr, t_sensorSurfaceVV[0].neighbor_vert, mappedSubSet, 0.2);
    std::cout << "SCDC duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeScdc<< " ms " << std::endl;

    //filter out bad MEG channels
    GeometryInfo::filterBadChannels(distanceMatrix, evoked.info, FIFFV_MEG_CH);

    //weight matrix
    qint64 startTimeWMat = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<SparseMatrix<float> > interpolationMatrix = Interpolation::createInterpolationMat(mappedSubSet,
                                                                                    distanceMatrix,
                                                                                    Interpolation::linear,
                                                                                    FLOAT_INFINITY);
    std::cout << "Weight matrix duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeWMat<< " ms " << std::endl;

    //realtime interpolation (1 iteration)
    VectorXd signal = VectorXd::Random(megSensors.size());
    qint64 startTimeRTI = QDateTime::currentMSecsSinceEpoch();
    Interpolation::interpolateSignal(*interpolationMatrix, signal.cast<float>());
    std::cout << "Real time interpol. : " << QDateTime::currentMSecsSinceEpoch() - startTimeRTI << " ms " << std::endl;

    return 0;
}
