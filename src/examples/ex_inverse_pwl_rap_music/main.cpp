//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief    Example of MNEForwardSolution and Powell Rap Music application
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>

#include <mne/mne.h>
#include <mne/mne_sourceestimate.h>

#include <inverse/rapMusic/pwlrapmusic.h>

#include <disp3D/viewers/abstractview.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>

#include <utils/mnemath.h>
#include <utils/generics/applicationlogger.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QVector3D>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;
using namespace DISP3DLIB;

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
    Q_INIT_RESOURCE(disp3d);
    #endif
    
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Compute Inverse Powell RAP-MUSIC Example");
    parser.addHelpOption();
    QCommandLineOption fwdFileOption("fwd", "Path to forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption evokedFileOption("ave", "Path to evoked <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption subjectDirectoryOption("subjDir", "Path to subject <directory>.", "directory", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");
    QCommandLineOption subjectOption("subj", "Selected <subject>.", "subject", "sample");
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");//"RapMusic.stc");
    QCommandLineOption numDipolePairsOption("numDip", "<number> of dipole pairs to localize.", "number", "1");
    QCommandLineOption doMovieOption("doMovie", "Create overlapping movie.", "doMovie", "false");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "orig");

    parser.addOption(fwdFileOption);
    parser.addOption(evokedFileOption);
    parser.addOption(subjectDirectoryOption);
    parser.addOption(subjectOption);
    parser.addOption(stcFileOption);
    parser.addOption(numDipolePairsOption);
    parser.addOption(doMovieOption);
    parser.addOption(annotOption);
    parser.addOption(surfOption);
    parser.process(app);

    // Parse command line parameters
    QFile t_fileFwd(parser.value(fwdFileOption));
    QFile t_fileEvoked(parser.value(evokedFileOption));
    QString subject(parser.value(subjectOption));
    QString subjectDir(parser.value(subjectDirectoryOption));
    AnnotationSet t_annotationSet(subject, 2, parser.value(annotOption), subjectDir);
    SurfaceSet t_surfSet(subject, 2, parser.value(surfOption), subjectDir);

    QString t_sFileNameStc(parser.value(stcFileOption));

    qint32 numDipolePairs = parser.value(numDipolePairsOption).toInt();

    bool doMovie = false;
    if(parser.value(doMovieOption) == "false" || parser.value(doMovieOption) == "0") {
        doMovie = false;
    } else if(parser.value(doMovieOption) == "true" || parser.value(doMovieOption) == "1") {
        doMovie = true;
    }

    qDebug() << "Start calculation with stc:" << t_sFileNameStc;

    // Load data
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    QStringList ch_sel_names = t_Fwd.info.ch_names;
    FiffEvoked pickedEvoked = evoked.pick_channels(ch_sel_names);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

//    std::cout << "Size " << t_clusteredFwd.sol->data.rows() << " x " << t_clusteredFwd.sol->data.cols() << std::endl;
//    std::cout << "Clustered Fwd:\n" << t_clusteredFwd.sol->data.row(0) << std::endl;

    PwlRapMusic t_pwlRapMusic(t_clusteredFwd, false, numDipolePairs);

    int iWinSize = 200;
    if(doMovie) {
        t_pwlRapMusic.setStcAttr(iWinSize, 0.6f);
    }

    MNESourceEstimate sourceEstimate = t_pwlRapMusic.calculateInverse(pickedEvoked);

    if(doMovie) {
        //Select only the activations once
        MatrixXd dataPicked(sourceEstimate.data.rows(), int(std::floor(sourceEstimate.data.cols()/iWinSize)));

        for(int i = 0; i < dataPicked.cols(); ++i) {
            dataPicked.col(i) = sourceEstimate.data.col(i*iWinSize);
        }

        sourceEstimate.data = dataPicked;
    }

    std::cout << "source estimated" << std::endl;

    if(sourceEstimate.isEmpty())
        return 1;

    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();

    p3DDataModel->addSurfaceSet(parser.value(subjectOption), evoked.comment, t_surfSet, t_annotationSet);

    //Add rt source loc data and init some visualization values
    if(MneDataTreeItem* pRTDataItem = p3DDataModel->addSourceData(parser.value(subjectOption),
                                                                  evoked.comment,
                                                                  sourceEstimate,
                                                                  t_clusteredFwd,
                                                                  t_surfSet,
                                                                  t_annotationSet)) {
        pRTDataItem->setLoopState(true);
        pRTDataItem->setTimeInterval(17);
        pRTDataItem->setNumberAverages(1);
        pRTDataItem->setStreamingState(true);
        pRTDataItem->setThresholds(QVector3D(0.01f,0.5f,1.0f));
        pRTDataItem->setVisualizationType("Annotation based");
        pRTDataItem->setColormapType("Hot");
    }

    p3DAbstractView->show();

    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        sourceEstimate.write(t_fileClusteredStc);
    }

    return app.exec();
}
