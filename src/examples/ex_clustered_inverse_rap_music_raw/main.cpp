//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Gabriel B Motta, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief    Example of the computation of a raw clustered inverse rap music
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
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_sourceestimate.h>

#include <inverse/rapMusic/rapmusic.h>

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
#include <QSet>
#include <QCommandLineParser>
#include <QVector3D>
#include <QRandomGenerator>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace DISP3DLIB;
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
    #ifdef STATICBUILD
    Q_INIT_RESOURCE(disp3d);
    #endif

    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Clustered Inverse Rap Music Raw Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");
    QCommandLineOption numDipolePairsOption("numDip", "<number> of dipole pairs to localize.", "number", "1");
    QCommandLineOption evokedIdxOption("aveIdx", "The average <index> to choose from the average file.", "index", "1");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption doMovieOption("doMovie", "Create overlapping movie.", "doMovie", "false");
    QCommandLineOption pickAllOption("pickAll", "Pick all channels.", "pickAll", "true");
    QCommandLineOption keepCompOption("keepComp", "Keep compensators.", "keepComp", "false");
    QCommandLineOption destCompsOption("destComps", "<Destination> of the compensator which is to be calculated.", "destination", "0");

    parser.addOption(inputOption);
    parser.addOption(eventsFileOption);
    parser.addOption(fwdOption);
    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(stcFileOption);
    parser.addOption(numDipolePairsOption);
    parser.addOption(evokedIdxOption);
    parser.addOption(hemiOption);
    parser.addOption(doMovieOption);
    parser.addOption(pickAllOption);
    parser.addOption(keepCompOption);
    parser.addOption(destCompsOption);

    parser.process(a);

    //Load data
    QFile t_fileRaw(parser.value(inputOption));
    QString t_sEventName = parser.value(eventsFileOption);
    QFile t_fileFwd(parser.value(fwdOption));

    SurfaceSet t_surfSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(surfOption), parser.value(subjectPathOption));
    AnnotationSet t_annotationSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));

    QString t_sFileNameStc(parser.value(stcFileOption));

    qint32 numDipolePairs = parser.value(numDipolePairsOption).toInt();

    //Choose average
    qint32 event = parser.value(evokedIdxOption).toInt();

    float tmin = -0.5f;
    float tmax = 0.5f;

    bool keep_comp = false;
    if(parser.value(keepCompOption) == "false" || parser.value(keepCompOption) == "0") {
        keep_comp = false;
    } else if(parser.value(keepCompOption) == "true" || parser.value(keepCompOption) == "1") {
        keep_comp = true;
    }

    fiff_int_t dest_comp = parser.value(destCompsOption).toInt();

    bool pick_all = false;
    if(parser.value(pickAllOption) == "false" || parser.value(pickAllOption) == "0") {
        pick_all = false;
    } else if(parser.value(pickAllOption) == "true" || parser.value(pickAllOption) == "1") {
        pick_all = true;
    }

    qint32 k, p;

    bool doMovie = false;
    if(parser.value(doMovieOption) == "false" || parser.value(doMovieOption) == "0") {
        doMovie = false;
    } else if(parser.value(doMovieOption) == "true" || parser.value(doMovieOption) == "1") {
        doMovie = true;
    }

    //
    // Load data
    //
    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    RowVectorXi picks;
    if (pick_all)
    {
        //
        // Pick all
        //
        picks.resize(raw.info.nchan);

        for(k = 0; k < raw.info.nchan; ++k)
            picks(k) = k;
        //
    }
    else
    {
        QStringList include;
        include << "STI 014";
        bool want_meg   = true;
        bool want_eeg   = false;
        bool want_stim  = false;

        picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);//prefer member function
    }

    QStringList ch_names;
    for(k = 0; k < picks.cols(); ++k)
        ch_names << raw.info.ch_names[picks(0,k)];

    //
    //   Set up projection
    //
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
//        fiff_int_t nproj = MNE::make_projector_info(raw.info, raw.proj); Using the member function instead
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)
        {
            printf("The projection vectors do not apply to these channels\n");
        }
        else
        {
            printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
        }
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
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
            return 0;
        }
    }
    //
    //  Read the events
    //
    QFile t_EventFile;
    MatrixXi events;
    if (t_sEventName.size() == 0)
    {
        p = t_fileRaw.fileName().indexOf(".fif");
        if (p > 0)
        {
            t_sEventName = t_fileRaw.fileName().replace(p, 4, "-eve.fif");
        }
        else
        {
            printf("Raw file name does not end properly\n");
            return 0;
        }
//        events = mne_read_events(t_sEventName);

        t_EventFile.setFileName(t_sEventName);
        MNE::read_events_from_fif(t_EventFile, events);
        printf("Events read from %s\n",t_sEventName.toUtf8().constData());
    }
    else
    {
        //
        //   Binary file
        //
        p = t_fileRaw.fileName().indexOf(".fif");
        if (p > 0)
        {
            t_EventFile.setFileName(t_sEventName);
            if(!MNE::read_events_from_fif(t_EventFile, events))
            {
                printf("Error while read events.\n");
                return 0;
            }
            printf("Binary event file %s read\n",t_sEventName.toUtf8().constData());
        }
        else
        {
            //
            //   Text file
            //
            printf("Text file %s is not supported jet.\n",t_sEventName.toUtf8().constData());
//            try
//                events = load(eventname);
//            catch
//                error(me,mne_omit_first_line(lasterr));
//            end
//            if size(events,1) < 1
//                error(me,'No data in the event file');
//            end
//            //
//            //   Convert time to samples if sample number is negative
//            //
//            for p = 1:size(events,1)
//                if events(p,1) < 0
//                    events(p,1) = events(p,2)*raw.info.sfreq;
//                end
//            end
//            //
//            //    Select the columns of interest (convert to integers)
//            //
//            events = int32(events(:,[1 3 4]));
//            //
//            //    New format?
//            //
//            if events(1,2) == 0 && events(1,3) == 0
//                fprintf(1,'The text event file %s is in the new format\n',eventname);
//                if events(1,1) ~= raw.first_samp
//                    error(me,'This new format event file is not compatible with the raw data');
//                end
//            else
//                fprintf(1,'The text event file %s is in the old format\n',eventname);
//                //
//                //   Offset with first sample
//                //
//                events(:,1) = events(:,1) + raw.first_samp;
//            end
        }
    }

    //
    //    Select the desired events
    //
    qint32 count = 0;
    MatrixXi selected = MatrixXi::Zero(1, events.rows());
    for (p = 0; p < events.rows(); ++p)
    {
        if (events(p,1) == 0 && events(p,2) == event)
        {
            selected(0,count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0)
        printf("%d matching events found\n",count);
    else
    {
        printf("No desired events found.\n");
        return 0;
    }

    fiff_int_t event_samp, from, to;
    MatrixXd timesDummy;

    MNEEpochDataList data;

    MNEEpochData* epoch = NULL;

    MatrixXd times;

    for (p = 0; p < count; ++p)
    {
        //
        //       Read a data segment
        //
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to   = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        epoch = new MNEEpochData();

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from, to, picks))
        {
            if (p == 0)
            {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = event;
            epoch->tmin = ((float)(from)-(float)(raw.first_samp))/raw.info.sfreq;
            epoch->tmax = ((float)(to)-(float)(raw.first_samp))/raw.info.sfreq;

            data.append(MNEEpochData::SPtr(epoch));//List takes ownwership of the pointer - no delete need
        }
        else
        {
            printf("Can't read the event data segments");
            return 0;
        }
    }

    if(data.size() > 0)
    {
        printf("Read %d epochs, %d samples each.\n",data.size(),(qint32)data[0]->epoch.cols());

        //DEBUG
        std::cout << data[0]->epoch.block(0,0,10,10) << std::endl;
        qDebug() << data[0]->epoch.rows() << " x " << data[0]->epoch.cols();

        std::cout << times.block(0,0,1,10) << std::endl;
        qDebug() << times.rows() << " x " << times.cols();
    }

    // Calculate the average
    // Option 1 - Random selection
    VectorXi vecSel(2);

    for(qint32 i = 0; i < vecSel.size(); ++i)
    {
        qint32 val = QRandomGenerator::global()->bounded(0,count);
        vecSel(i) = val;
    }

//    //Option 2 - Take all epochs
//    VectorXi vecSel(data.size());

//    for(qint32 i = 0; i < vecSel.size(); ++i)
//    {
//        vecSel(i) = i;
//    }

//    //Option 3 - Manual selection
//    VectorXi vecSel(15);
//    vecSel << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15;
    std::cout << "Select following epochs to average:\n" << vecSel << std::endl;

    FiffEvoked evoked = data.average(raw.info, tmin*raw.info.sfreq, floor(tmax*raw.info.sfreq + 0.5), vecSel);

    QStringList ch_sel_names = t_Fwd.info.ch_names;
    FiffEvoked pickedEvoked = evoked.pick_channels(ch_sel_names);

    //########################################################################################
    // RAP MUSIC Source Estimate

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

    //
    // Compute inverse solution
    //
    RapMusic t_rapMusic(t_clusteredFwd, false, numDipolePairs);

    int iWinSize = 200;
    if(doMovie) {
        t_rapMusic.setStcAttr(iWinSize, 0.6f);
    }

    MNESourceEstimate sourceEstimate = t_rapMusic.calculateInverse(pickedEvoked);

    if(doMovie) {
        //Select only the activations once
        MatrixXd dataPicked(sourceEstimate.data.rows(), int(std::floor(sourceEstimate.data.cols()/iWinSize)));

        for(int i = 0; i < dataPicked.cols(); ++i) {
            dataPicked.col(i) = sourceEstimate.data.col(i*iWinSize);
        }

        sourceEstimate.data = dataPicked;
    }

    if(sourceEstimate.isEmpty()) {
        return 1;
    }

//    // View activation time-series
//    std::cout << "\nsourceEstimate:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
//    std::cout << "time\n" << sourceEstimate.times.block(0,0,1,10) << std::endl;
//    std::cout << "timeMin\n" << sourceEstimate.times[0] << std::endl;
//    std::cout << "timeMax\n" << sourceEstimate.times[sourceEstimate.times.size()-1] << std::endl;
//    std::cout << "time step\n" << sourceEstimate.tstep << std::endl;

    //Source Estimate end
    //########################################################################################

//    //only one time point - P100
//    qint32 sample = 0;
//    for(qint32 i = 0; i < sourceEstimate.times.size(); ++i)
//    {
//        if(sourceEstimate.times(i) >= 0)
//        {
//            sample = i;
//            break;
//        }
//    }
//    sample += (qint32)ceil(0.106/sourceEstimate.tstep); //100ms
//    sourceEstimate = sourceEstimate.reduce(sample, 1);

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

    return a.exec();//1;//a.exec();
}
