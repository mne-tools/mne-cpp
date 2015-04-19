//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2013
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of the computation of a rawClusteredInverse
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>
#include <mne/mne.h>

#include <mne/mne_epoch_data_list.h>

#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

#include <disp3D/inverseview.h>

#include <utils/mnemath.h>

#include <iostream>

#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGuiApplication>
#include <QSet>
#include <QElapsedTimer>
#include <QCommandLineParser>

//#define BENCHMARK


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace DISP3DLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// Enums
//=============================================================================================================

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
* The function main marks the entry point of the program.
* By default, main has the storage class extern.
*
* @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("MNE ROI Cluster Evaluation");
    QGuiApplication::setApplicationVersion("Revision 2");

    ///////////////////////////////////// #1 CLI Parser /////////////////////////////////////
    QCommandLineParser parser;
    parser.setApplicationDescription("MNE ROI Cluster Evaluation");
    parser.addHelpOption();
    parser.addVersionOption();

    // MEG Source Directory
    QCommandLineOption srcDirectoryOption(QStringList() << "s" << "meg-source-directory",
            QCoreApplication::translate("main", "Read MEG (fwd, cov, raw, eve) source files from <directory>."),
            QCoreApplication::translate("main", "directory"),
            "./MNE-sample-data/MEG/sample/");
    parser.addOption(srcDirectoryOption);

    // Forward Solution File
    QCommandLineOption fwdFileOption(QStringList() << "fwd" << "forward-solution",
            QCoreApplication::translate("main", "The forward solution <file>."),
            QCoreApplication::translate("main", "file"),
            "sample_audvis-meg-eeg-oct-6-fwd.fif");
    parser.addOption(fwdFileOption);

    // Fixed Forward Solution File
    QCommandLineOption xfwdFileOption(QStringList() << "xfwd" << "fixed-forward-solution",
            QCoreApplication::translate("main", "The fixed forward solution <file>."),
            QCoreApplication::translate("main", "file"),
            "sample_audvis-meg-eeg-oct-6-fwd-fixed.fif");//"D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-6-fwd-fixed.fif");
    parser.addOption(xfwdFileOption);

    // Covariance File
    QCommandLineOption covFileOption(QStringList() << "cov" << "covariance",
            QCoreApplication::translate("main", "The covariance <file>."),
            QCoreApplication::translate("main", "file"),
            "sample_audvis-cov.fif");
    parser.addOption(covFileOption);

    // Raw MEG File
    QCommandLineOption rawFileOption(QStringList() << "raw" << "raw-file",
            QCoreApplication::translate("main", "The raw MEG data <file>."),
            QCoreApplication::translate("main", "file"),
            "sample_audvis_raw.fif");
    parser.addOption(rawFileOption);

    // Event File
    QCommandLineOption eveFileOption(QStringList() << "eve" << "event-file",
            QCoreApplication::translate("main", "The event <file>."),
            QCoreApplication::translate("main", "file"),
            "sample_audvis_raw-eve.fif");
    parser.addOption(eveFileOption);

    // Event Num
    QCommandLineOption evenNumOption(QStringList() << "evenum" << "event-number",
            QCoreApplication::translate("main", "The <event number>."),
            QCoreApplication::translate("main", "event"),
            "1");//2;//3;//4;
    parser.addOption(evenNumOption);

    // FS Subject Directory
    QCommandLineOption subjDirectoryOption(QStringList() << "subjdir" << "subject-directory",
            QCoreApplication::translate("main", "The FreeSurfer <subjects directory>."),
            QCoreApplication::translate("main", "directory"),
            "./MNE-sample-data/subjects");
    parser.addOption(subjDirectoryOption);

    // FS Subject
    QCommandLineOption subjIdOption(QStringList() << "subjid" << "subject-id",
            QCoreApplication::translate("main", "The FreeSurfer <subject id>."),
            QCoreApplication::translate("main", "subject id"),
            "sample");
    parser.addOption(subjIdOption);

    // Target Directory
    QCommandLineOption targetDirectoryOption(QStringList() << "t" << "target-directory",
            QCoreApplication::translate("main", "Copy all result files into <directory>."),
            QCoreApplication::translate("main", "directory"));
    parser.addOption(targetDirectoryOption);

    // Target Prefix
    QCommandLineOption targetPrefixOption(QStringList() << "p" << "prefix",
            QCoreApplication::translate("main", "The result file's <prefix>."),
            QCoreApplication::translate("main", "prefix"));
    parser.addOption(targetPrefixOption);

    //// Source estimate parameters ////
    // SNR
    QCommandLineOption snrOption(QStringList() << "snr" << "signal-to-noise-ratio",
            QCoreApplication::translate("main", "The <snr> estimation for the given data file."),
            QCoreApplication::translate("main", "snr"),
            "1.0");//0.1f;//1.0f;//3.0f;
    parser.addOption(snrOption);

    // METHOD
    QCommandLineOption methodOption(QStringList() << "m" << "method",
            QCoreApplication::translate("main", "The estimation <method>."),
            QCoreApplication::translate("main", "method"),
            "dSPM");//"MNE" | "dSPM" | "sLORETA"
    parser.addOption(methodOption);

    // File Name Clustered Inverse Operator
    QCommandLineOption clustInvFileOption(QStringList() << "icf" << "inverse-clustered-file",
            QCoreApplication::translate("main", "Target <file> to store clustered inverse operator to."),
            QCoreApplication::translate("main", "file"));
    parser.addOption(clustInvFileOption);

    // File Name of the Source Estimate
    QCommandLineOption stcFileOption(QStringList() << "stcf" << "stc-file",
            QCoreApplication::translate("main", "Target <stcfile> to store stc to."),
            QCoreApplication::translate("main", "file"));//"mind006_051209_auditory01.stc"
    parser.addOption(stcFileOption);


    // Process the actual command line arguments given by the user
    parser.process(app);


    //////////////////////////////// #2 get parsed values /////////////////////////////////

    //Sources
    QString sFwdName = parser.value(srcDirectoryOption)+parser.value(fwdFileOption);
    qDebug() << "Forward Solution" << sFwdName;

    QString sXFwdName = parser.value(srcDirectoryOption)+parser.value(xfwdFileOption);
    qDebug() << "Fixed Forward Solution" << sXFwdName;
//    QFile t_fileXFwd(sXFwdName);

    QString sCovName = parser.value(srcDirectoryOption)+parser.value(covFileOption);
    qDebug() << "Covariance matrix" << sCovName;

    QString sRawName = parser.value(srcDirectoryOption)+parser.value(rawFileOption);
    qDebug() << "Raw data" << sRawName;

    QString t_sEventName = parser.value(srcDirectoryOption)+parser.value(eveFileOption);
    qDebug() << "Events" << t_sEventName;

    qint32 eveNum = (qint32)parser.value(evenNumOption).toInt();
    qDebug() << "Event Number" << eveNum;

    QString t_sSubjectsDir = parser.value(subjDirectoryOption);
    qDebug() << "Subjects Directory" << t_sSubjectsDir;

    QString t_sSubject = parser.value(subjIdOption);
    qDebug() << "Subject" << t_sSubject;

    AnnotationSet t_annotationSet(t_sSubject, 2, "aparc.a2009s", t_sSubjectsDir);
    SurfaceSet t_surfSet(t_sSubject, 2, "white", t_sSubjectsDir);

    //Targets
    QString sTargetDir = parser.value(targetDirectoryOption);
    qDebug() << "Target Directory" << sTargetDir;

    QString sTargetPrefix = parser.value(targetPrefixOption);
    qDebug() << "Target Prefix" << sTargetPrefix;

    //// Source estimate parameters ////
    double snr = parser.value(snrOption).toFloat();
    qDebug() << "SNR" << snr;

    QString method = parser.value(methodOption);
    qDebug() << "Method" << method;

    QString t_sFileNameClusteredInv = parser.value(clustInvFileOption);
    qDebug() << "Store clustered inverse operator to:" << t_sFileNameClusteredInv;

    QString t_sFileNameStc = parser.value(stcFileOption);
    qDebug() << "Store stc to:" << t_sFileNameStc;




    //OLD
//    QFile t_fileFwd("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
//    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
//    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
//    QString t_sEventName = "./MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif";
//    AnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");
//    SurfaceSet t_surfSet("sample", 2, "white", "./MNE-sample-data/subjects");

//    QFile t_fileFwd("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-fwd.fif");
//    QFile t_fileCov("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-cov.fif");
//    QFile t_fileRaw("D:/Data/MEG/mind006/mind006_051209_auditory01_raw.fif");
//    QString t_sEventName = "D:/Data/MEG/mind006/mind006_051209_auditory01_raw-eve.fif";
//    AnnotationSet t_annotationSet("mind006", 2, "aparc.a2009s", "D:/Data/subjects");
//    SurfaceSet t_surfSet("mind006", 2, "white", "D:/Data/subjects");


    ///////////////////////////////////// #3 read data //////////////////////////////////////
    qint32 event = eveNum;//1;

    float tmin = -0.2f;
    float tmax = 0.4f;

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;
    bool pick_all  = true;

    qint32 k, p;

    //
    //   Setup for reading the raw data
    //
    QFile t_fileRaw(sRawName);
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

        picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
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
        t_EventFile.setFileName(t_sEventName);
        MNE::read_events(t_EventFile, events);
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
            if(!MNE::read_events(t_EventFile, events))
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
//    std::cout << "Events:\n" << events << std::endl;
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

    //DEBUG Output
    if(data.size() > 0)
    {
        printf("Sampling frequency, %f\n", raw.info.sfreq);

        printf("Read %d epochs, %d samples each.\n",data.size(),(qint32)data[0]->epoch.cols());
//        //DEBUG
//        std::cout << data[0]->epoch.block(0,0,10,10) << std::endl;
//        qDebug() << data[0]->epoch.rows() << " x " << data[0]->epoch.cols();

//        std::cout << times.block(0,0,1,10) << std::endl;
//        qDebug() << times.rows() << " x " << times.cols();
    }

    /////////////////////////////////// #4 process data /////////////////////////////////////

    //
    // calculate the average
    //
    FiffEvoked evoked = data.average(raw.info, tmin*raw.info.sfreq, floor(tmax*raw.info.sfreq + 0.5));


    //########################################################################################
    // Source Estimate

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

//    // Load data
//    fiff_int_t setno = 1;
//    QPair<QVariant, QVariant> baseline(QVariant(), 0);
//    FiffEvoked evoked(t_fileEvoked, setno, baseline);
//    if(evoked.isEmpty())
//        return 1;

    QFile t_fileFwd(sFwdName);
    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    QFile t_fileFwdFixed(sXFwdName);
    MNEForwardSolution t_FwdFixed(t_fileFwdFixed);
    if(t_FwdFixed.isEmpty())
        return 1;

    QFile t_fileCov(sCovName);
    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);


    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 1. t_Fwd <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

    QString sG = sTargetDir + sTargetPrefix + QString("G.txt");
    std::ofstream ofs_G(sG.toUtf8().constData(), std::ofstream::out);
    if (ofs_G.is_open())
    {
        printf("writing to %s\n",sG.toUtf8().constData());
        ofs_G << t_Fwd.sol->data << '\n';
    }
    else
        printf("Not writing to %s\n",sG.toUtf8().constData());
    ofs_G.close();

//    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 1. t_Fwd_whitened <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
//    //
//    // Whiten L1 gain matrix
//    //
//    MatrixXd t_Fwd_whitened(0,0);

//    //
//    // Whiten gain matrix before clustering -> cause diffenerent units Magnetometer, Gradiometer and EEG
//    //
//    if(!noise_cov.isEmpty() && !evoked.info.isEmpty())
//    {
//        FiffInfo p_outFwdInfo;
//        FiffCov p_outNoiseCov;
//        MatrixXd p_outWhitener;
//        qint32 p_outNumNonZero;
//        //do whitening with noise cov
//        t_Fwd.prepare_forward(evoked.info, noise_cov, false, p_outFwdInfo, t_Fwd_whitened, p_outNoiseCov, p_outWhitener, p_outNumNonZero);
//        printf("\tWhitening the forward solution.\n");

//        t_Fwd_whitened = p_outWhitener*t_Fwd_whitened;
//    }

//    QString sG_Whitened = sTargetDir + sTargetPrefix + QString("G_whitened.txt");
//    std::ofstream ofs_G_Whitened(sG_Whitened.toUtf8().constData(), std::ofstream::out);//"G_whitened.txt", std::ofstream::out);
//    if (ofs_G_Whitened.is_open())
//    {
//        printf("writing to %s\n",sG_Whitened.toUtf8().constData());
//        ofs_G_Whitened << t_Fwd_whitened << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sG_Whitened.toUtf8().constData());
//    ofs_G_Whitened.close();

//    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //
    // Cluster forward solution;
    //
    MatrixXd D_L1;

    std::cout << "t_Fwd " << t_Fwd.sol->data.rows() << " x " << t_Fwd.sol->data.cols() << std::endl;

    MNEForwardSolution t_clusteredFwd_L1 = t_Fwd.cluster_forward_solution(t_annotationSet, 20, D_L1, noise_cov, evoked.info, "cityblock");
    QString sCILHL1 = sTargetDir + sTargetPrefix + QString("ClusterInfoLH_L1.txt");
    t_clusteredFwd_L1.src[0].cluster_info.write(sCILHL1);
    QString sCIRHL1 = sTargetDir + sTargetPrefix + QString("ClusterInfoRH_L1.txt");
    t_clusteredFwd_L1.src[1].cluster_info.write(sCIRHL1);


    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 2. t_clusteredFwd_L1 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

    QString sG_L1 = sTargetDir + sTargetPrefix + QString("G_L1.txt");
    std::ofstream ofs_G_L1(sG_L1.toUtf8().constData(), std::ofstream::out);
    if (ofs_G_L1.is_open())
    {
        printf("writing to %s\n",sG_L1.toUtf8().constData());
        ofs_G_L1 << t_clusteredFwd_L1.sol->data << '\n';
    }
    else
        printf("Not writing to %s\n",sG_L1.toUtf8().constData());
    ofs_G_L1.close();


//    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 2. t_clusteredFwd_L1_whitened <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
//    //
//    // Whiten L1 gain matrix
//    //
//    MatrixXd t_clusteredFwd_L1_whitened(0,0);

//    //
//    // Whiten gain matrix before clustering -> cause diffenerent units Magnetometer, Gradiometer and EEG
//    //
//    if(!noise_cov.isEmpty() && !evoked.info.isEmpty())
//    {
//        FiffInfo p_outFwdInfo;
//        FiffCov p_outNoiseCov;
//        MatrixXd p_outWhitener;
//        qint32 p_outNumNonZero;
//        //do whitening with noise cov
//        t_clusteredFwd_L1.prepare_forward(evoked.info, noise_cov, false, p_outFwdInfo, t_clusteredFwd_L1_whitened, p_outNoiseCov, p_outWhitener, p_outNumNonZero);
//        printf("\tWhitening the forward solution.\n");

//        t_clusteredFwd_L1_whitened = p_outWhitener*t_clusteredFwd_L1_whitened;
//    }

//    QString sG_L1_Whitened = sTargetDir + sTargetPrefix + QString("G_L1_whitened.txt");
//    std::ofstream ofs_G_L1_Whitened(sG_L1_Whitened.toUtf8().constData(), std::ofstream::out);//"G_L1_whitened.txt", std::ofstream::out);
//    if (ofs_G_L1_Whitened.is_open())
//    {
//        printf("writing to %s\n",sG_L1_Whitened.toUtf8().constData());
//        ofs_G_L1_Whitened << t_clusteredFwd_L1_whitened << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sG_L1_Whitened.toUtf8().constData());
//    ofs_G_L1_Whitened.close();

//    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    std::cout << "D_L1 " << D_L1.rows() << " x " << D_L1.cols() << std::endl;
    std::cout << "t_clusteredFwd_L1 " << t_clusteredFwd_L1.sol->data.rows() << " x " << t_clusteredFwd_L1.sol->data.cols() << std::endl;

    MatrixXd D_L2;

    MNEForwardSolution t_clusteredFwd_L2 = t_Fwd.cluster_forward_solution(t_annotationSet, 20, D_L2, noise_cov, evoked.info, "sqeuclidean");
    QString sCILH_L2 = sTargetDir + sTargetPrefix + QString("ClusterInfoLH_L2.txt");
    t_clusteredFwd_L2.src[0].cluster_info.write(sCILH_L2);
    QString sCIRH_L2 = sTargetDir + sTargetPrefix + QString("ClusterInfoRH_L2.txt");
    t_clusteredFwd_L2.src[1].cluster_info.write(sCIRH_L2);

    std::cout << "D_L2 " << D_L2.rows() << " x " << D_L2.cols() << std::endl;
    std::cout << "t_clusteredFwd_L2 " << t_clusteredFwd_L2.sol->data.rows() << " x " << t_clusteredFwd_L2.sol->data.cols() << std::endl;


    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 3. t_clusteredFwd_L2 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

    QString sG_L2 = sTargetDir + sTargetPrefix + QString("G_L2.txt");
    std::ofstream ofs_G_L2(sG_L2.toUtf8().constData(), std::ofstream::out);
    if (ofs_G_L2.is_open())
    {
        printf("writing to %s\n",sG_L2.toUtf8().constData());
        ofs_G_L2 << t_clusteredFwd_L2.sol->data << '\n';
    }
    else
        printf("Not writing to %s\n",sG_L2.toUtf8().constData());
    ofs_G_L2.close();

//    qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 3. t_clusteredFwd_L2_whitened <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
//    //
//    // Whiten L1 gain matrix
//    //
//    MatrixXd t_clusteredFwd_L2_whitened(0,0);

//    //
//    // Whiten gain matrix before clustering -> cause diffenerent units Magnetometer, Gradiometer and EEG
//    //
//    if(!noise_cov.isEmpty() && !evoked.info.isEmpty())
//    {
//        FiffInfo p_outFwdInfo;
//        FiffCov p_outNoiseCov;
//        MatrixXd p_outWhitener;
//        qint32 p_outNumNonZero;
//        //do whitening with noise cov
//        t_clusteredFwd_L2.prepare_forward(evoked.info, noise_cov, false, p_outFwdInfo, t_clusteredFwd_L2_whitened, p_outNoiseCov, p_outWhitener, p_outNumNonZero);
//        printf("\tWhitening the forward solution.\n");

//        t_clusteredFwd_L2_whitened = p_outWhitener*t_clusteredFwd_L2_whitened;
//    }

//    QString sG_L2_Whitened = sTargetDir + sTargetPrefix + QString("G_L2_whitened.txt");
//    std::ofstream ofs_G_L2_Whitened(sG_L2_Whitened.toUtf8().constData(), std::ofstream::out);//"G_L2_whitened.txt", std::ofstream::out);
//    if (ofs_G_L2_Whitened.is_open())
//    {
//        printf("writing to %s\n",sG_L2_Whitened.toUtf8().constData());
//        ofs_G_L2_Whitened << t_clusteredFwd_L2_whitened << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sG_L2_Whitened.toUtf8().constData());
//    ofs_G_L2_Whitened.close();

    return CommandLineOk;

//    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;

    QFile t_fileSelectedFwd(sFwdName);


//    QFile t_fileFwd("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-fwd.fif");
//    QFile t_fileCov("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-cov.fif");
//    QFile t_fileRaw("D:/Data/MEG/mind006/mind006_051209_auditory01_raw.fif");
//    QString t_sEventName = "D:/Data/MEG/mind006/mind006_051209_auditory01_raw-eve.fif";
//    AnnotationSet t_annotationSet("mind006", 2, "aparc.a2009s", "D:/Data/subjects");
//    SurfaceSet t_surfSet("mind006", 2, "white", "D:/Data/subjects");


    MNEForwardSolution t_selectedRawFwd(t_fileSelectedFwd);
    if(t_selectedRawFwd.isEmpty())
        return 1;

    MatrixXd D_selected;
    MNEForwardSolution t_selectedFwd = t_selectedRawFwd.reduce_forward_solution(t_clusteredFwd_L2.isFixedOrient() ? t_clusteredFwd_L2.sol->data.cols() : t_clusteredFwd_L2.sol->data.cols()/3, D_selected);

//    qDebug() << "#### t_selectedFwd" << t_selectedFwd.sol->data.rows() << "x" << t_selectedFwd.sol->data.cols();

    MNEInverseOperator inverse_operator_selected(info, t_selectedFwd, noise_cov, 0.2f, 0.8f);

    MNEInverseOperator inverse_operator_clustered_L1(info, t_clusteredFwd_L1, noise_cov, 0.2f, 0.8f);

    MNEInverseOperator inverse_operator_clustered_L2(info, t_clusteredFwd_L2, noise_cov, 0.2f, 0.8f);

    MNEInverseOperator inverse_operator(info, t_Fwd, noise_cov, 0.2f, 0.8f);

//    //
//    // save clustered inverse
//    //
//    if(!t_sFileNameClusteredInv.isEmpty())
//    {
//        QFile t_fileClusteredInverse(t_sFileNameClusteredInv);
//        inverse_operator_clustered_L2.write(t_fileClusteredInverse);
//    }


    //
    // Compute inverse solution
    //
    MinimumNorm minimumNormSelected(inverse_operator_selected, lambda2, method);

    MinimumNorm minimumNormClustered_L1(inverse_operator_clustered_L1, lambda2, method);

    MinimumNorm minimumNormClustered_L2(inverse_operator_clustered_L2, lambda2, method);

    MinimumNorm minimumNorm(inverse_operator, lambda2, method);


//#ifdef BENCHMARK
//    //
//    //   Set up the inverse according to the parameters
//    //
//    minimumNormClustered.doInverseSetup(vecSel.size(),false);

//    MNESourceEstimate sourceEstimate;
//    QList<qint64> qVecElapsedTime;
//    for(qint32 i = 0; i < 100; ++i)
//    {
//        //Benchmark time
//        QElapsedTimer timer;
//        timer.start();
//        sourceEstimate = minimumNormClustered.calculateInverse(evoked.data, evoked.times(0), evoked.times(1)-evoked.times(0));
//        qVecElapsedTime.append(timer.elapsed());
//    }

//    double meanTime = 0.0;
//    qint32 offset = 19;
//    qint32 c = 0;
//    for(qint32 i = offset; i < qVecElapsedTime.size(); ++i)
//    {
//        meanTime += qVecElapsedTime[i];
//        ++c;
//    }

//    meanTime /= (double)c;

//    double varTime = 0;
//    for(qint32 i = offset; i < qVecElapsedTime.size(); ++i)
//        varTime += pow(qVecElapsedTime[i] - meanTime,2);

//    varTime /= (double)c - 1.0f;
//    varTime = sqrt(varTime);

//    qDebug() << "MNE calculation took" << meanTime << "+-" << varTime << "ms in average";

//#else
//    MNESourceEstimate sourceEstimateSelected =

    minimumNormSelected.calculateInverse(evoked);

//    MNESourceEstimate sourceEstimateClustered_L1 =

    minimumNormClustered_L1.calculateInverse(evoked);

//    MNESourceEstimate sourceEstimateClustered_L2 =

    minimumNormClustered_L2.calculateInverse(evoked);

//    MNESourceEstimate sourceEstimate =

    minimumNorm.calculateInverse(evoked);
//#endif


//    ////////////////////////////////// original
//    // #### R calculation ####
//    printf("R original calculation\n");
//    MatrixXd M_orig = minimumNorm.getKernel();

//    MatrixXd R_orig = M_orig * t_FwdFixed.sol->data;

//    QString sROrig = sTargetDir + sTargetPrefix + QString("R_orig.txt");
//    std::ofstream ofs_R_orig(sROrig.toUtf8().constData(), std::ofstream::out);//, std::ofstream::out);
//    if (ofs_R_orig.is_open())
//    {
//        printf("writing to %s\n", sROrig.toUtf8().constData());
//        ofs_R_orig << R_orig << '\n';
//    }
//    else
//        printf("Not writing to %s\n", sROrig.toUtf8().constData());
//    ofs_R_orig.close();

//    R_orig.resize(0,0);


//    //ToDo:just original
//    return CommandLineOk;


    ////////////////////////////////// L1 calculations

    MatrixXd D_MT_L1;
    MatrixXd MT_clustered_L1 = minimumNorm.getPreparedInverseOperator().cluster_kernel(t_annotationSet, 20, D_MT_L1, "cityblock");

    // #### R calculation ####
    //// Option II_L1
    //Cluster Inverse operator
    MatrixXd M_L1 = D_L1.transpose() * minimumNorm.getKernel();

    MatrixXd R_L1 = M_L1 * t_FwdFixed.sol->data;


//    QString sTargetDir = parser.value(targetDirectoryOption);
//    qDebug() << "Target Directory" << sTargetDir;

//    QString sTargetPrefix = parser.value(targetPrefixOption);
//    qDebug() << "Target Prefix" << sTargetPrefix;


    QString sR_L1 = sTargetDir + sTargetPrefix + QString("R_L1.txt");

    std::ofstream ofs_R_L1(sR_L1.toUtf8().constData(), std::ofstream::out);//"R_L1.txt", std::ofstream::out);
    if (ofs_R_L1.is_open())
    {
        printf("writing to %s\n",sR_L1.toUtf8().constData());

        ofs_R_L1 << R_L1 << '\n';
    }
    else
        printf("Not writing to %s\n",sR_L1.toUtf8().constData());
    ofs_R_L1.close();

    M_L1.resize(0,0);
    R_L1.resize(0,0);

    //// Option I_L1
    printf("[3]\n");
    MatrixXd M_clusterd_L1 = minimumNormClustered_L1.getKernel();

    printf("[4]\n");
    MatrixXd R_clustered_L1 = M_clusterd_L1 * t_FwdFixed.sol->data;

    QString sRClustered_L1 = sTargetDir + sTargetPrefix + QString("R_clustered_L1.txt");
    std::ofstream ofs_R_clustered_L1(sRClustered_L1.toUtf8().constData(), std::ofstream::out);//, std::ofstream::out);
    if (ofs_R_clustered_L1.is_open())
    {
        printf("writing to %s\n", sRClustered_L1.toUtf8().constData());
        ofs_R_clustered_L1 << R_clustered_L1 << '\n';
    }
    else
        printf("Not writing to %s\n", sRClustered_L1.toUtf8().constData());
    ofs_R_clustered_L1.close();

    M_clusterd_L1.resize(0,0);
    R_clustered_L1.resize(0,0);

//Cluster Operator D_L1
    QString sD_L1 = sTargetDir + sTargetPrefix + QString("D_L1.txt");
    std::ofstream ofs_D_L1(sD_L1.toUtf8().constData(), std::ofstream::out);//"D_L1.txt", std::ofstream::out);
    if (ofs_D_L1.is_open())
    {
        printf("writing to %s\n",sD_L1.toUtf8().constData());
        ofs_D_L1 << D_L1 << '\n';
    }
    else
        printf("Not writing to %s\n",sD_L1.toUtf8().constData());
    ofs_D_L1.close();


    //// Option III_L1
    printf("[5]\n");
    MatrixXd R_MT_clustered_L1 = MT_clustered_L1.transpose() * t_FwdFixed.sol->data;

    QString sRMTClust_L1 = sTargetDir + sTargetPrefix + QString("R_MT_clustered_L1.txt");
    std::ofstream ofs_R_MT_clustered_L1(sRMTClust_L1.toUtf8().constData(), std::ofstream::out);
    if (ofs_R_MT_clustered_L1.is_open())
    {
        printf("writing to %s\n",sRMTClust_L1.toUtf8().constData());
        ofs_R_MT_clustered_L1 << R_MT_clustered_L1 << '\n';
    }
    else
        printf("Not writing to %s\n",sRMTClust_L1.toUtf8().constData());
    ofs_R_MT_clustered_L1.close();

    R_MT_clustered_L1.resize(0,0);

    //Cluster Operator D
    QString sDMT_L1 = sTargetDir + sTargetPrefix + QString("D_MT_L1.txt");
    std::ofstream ofs_D_MT_L1(sDMT_L1.toUtf8().constData(), std::ofstream::out);
    if (ofs_D_MT_L1.is_open())
    {
        printf("writing to %s\n",sDMT_L1.toUtf8().constData());
        ofs_D_MT_L1 << D_MT_L1 << '\n';
    }
    else
        printf("Not writing to %s\n",sDMT_L1.toUtf8().constData());
    ofs_D_MT_L1.close();





    ////////////////////////////////// L2 calculations

    MatrixXd D_MT_L2;
    MatrixXd MT_clustered_L2 = minimumNorm.getPreparedInverseOperator().cluster_kernel(t_annotationSet, 20, D_MT_L2, "sqeuclidean");

    // #### R calculation ####

    //// Option II_L2
    MatrixXd M_L2 = D_L2.transpose() * minimumNorm.getKernel();

    MatrixXd R_L2 = M_L2 * t_FwdFixed.sol->data;


//    QString sTargetDir = parser.value(targetDirectoryOption);
//    qDebug() << "Target Directory" << sTargetDir;

//    QString sTargetPrefix = parser.value(targetPrefixOption);
//    qDebug() << "Target Prefix" << sTargetPrefix;


    QString sR_L2 = sTargetDir + sTargetPrefix + QString("R_L2.txt");

    std::ofstream ofs_R_L2(sR_L2.toUtf8().constData(), std::ofstream::out);//"R_L2.txt", std::ofstream::out);
    if (ofs_R_L2.is_open())
    {
        printf("writing to %s\n",sR_L2.toUtf8().constData());

        ofs_R_L2 << R_L2 << '\n';
    }
    else
        printf("Not writing to %s\n",sR_L2.toUtf8().constData());
    ofs_R_L2.close();

    M_L2.resize(0,0);
    R_L2.resize(0,0);


    //// Option I_L2
    printf("[3]\n");
    MatrixXd M_clusterd_L2 = minimumNormClustered_L2.getKernel();

    printf("[4]\n");
    MatrixXd R_clustered_L2 = M_clusterd_L2 * t_FwdFixed.sol->data;

    QString sRClustered_L2 = sTargetDir + sTargetPrefix + QString("R_clustered_L2.txt");
    std::ofstream ofs_R_clustered_L2(sRClustered_L2.toUtf8().constData(), std::ofstream::out);//, std::ofstream::out);
    if (ofs_R_clustered_L2.is_open())
    {
        printf("writing to %s\n", sRClustered_L2.toUtf8().constData());
        ofs_R_clustered_L2 << R_clustered_L2 << '\n';
    }
    else
        printf("Not writing to %s\n", sRClustered_L2.toUtf8().constData());
    ofs_R_clustered_L2.close();

    M_clusterd_L2.resize(0,0);
    R_clustered_L2.resize(0,0);

//Cluster Operator D_L2
    QString sD_L2 = sTargetDir + sTargetPrefix + QString("D_L2.txt");
    std::ofstream ofs_D_L2(sD_L2.toUtf8().constData(), std::ofstream::out);//"D_L2.txt", std::ofstream::out);
    if (ofs_D_L2.is_open())
    {
        printf("writing to %s\n",sD_L2.toUtf8().constData());
        ofs_D_L2 << D_L2 << '\n';
    }
    else
        printf("Not writing to %s\n",sD_L2.toUtf8().constData());
    ofs_D_L2.close();


    //// Option III_L2
    printf("[5]\n");
    MatrixXd R_MT_clustered_L2 = MT_clustered_L2.transpose() * t_FwdFixed.sol->data;

    QString sRMTClust_L2 = sTargetDir + sTargetPrefix + QString("R_MT_clustered_L2.txt");
    std::ofstream ofs_R_MT_clustered_L2(sRMTClust_L2.toUtf8().constData(), std::ofstream::out);
    if (ofs_R_MT_clustered_L2.is_open())
    {
        printf("writing to %s\n",sRMTClust_L2.toUtf8().constData());
        ofs_R_MT_clustered_L2 << R_MT_clustered_L2 << '\n';
    }
    else
        printf("Not writing to %s\n",sRMTClust_L2.toUtf8().constData());
    ofs_R_MT_clustered_L2.close();

    R_MT_clustered_L2.resize(0,0);

    //Cluster Operator D
    QString sDMT_L2 = sTargetDir + sTargetPrefix + QString("D_MT_L2.txt");
    std::ofstream ofs_D_MT_L2(sDMT_L2.toUtf8().constData(), std::ofstream::out);
    if (ofs_D_MT_L2.is_open())
    {
        printf("writing to %s\n",sDMT_L2.toUtf8().constData());
        ofs_D_MT_L2 << D_MT_L2 << '\n';
    }
    else
        printf("Not writing to %s\n",sDMT_L2.toUtf8().constData());
    ofs_D_MT_L2.close();





    ////////////////////////////////// Selection calculation

    //option d)
    MatrixXd M_selected = minimumNormSelected.getKernel();

//    qDebug() << "M_selected: " << M_selected.rows() << "x" << M_selected.cols();


    printf("[7]\n");
    MatrixXd R_selected= M_selected * t_FwdFixed.sol->data;

    QString sRselected = sTargetDir + sTargetPrefix + QString("R_selected.txt");
    std::ofstream ofs_R_selected(sRselected.toUtf8().constData(), std::ofstream::out);//"R_selected.txt", std::ofstream::out);
    if (ofs_R_selected.is_open())
    {
        printf("writing to %s\n",sRselected.toUtf8().constData());
        ofs_R_selected << R_selected << '\n';
    }
    else
        printf("Not writing to %s\n",sRselected.toUtf8().constData());
    ofs_R_selected.close();

    R_selected.resize(0,0);

    //Cluster Operator D
    QString sDselected = sTargetDir + sTargetPrefix + QString("D_selected.txt");
    std::ofstream ofs_D_selected(sDselected.toUtf8().constData(), std::ofstream::out);//"D_selected.txt", std::ofstream::out);
    if (ofs_D_selected.is_open())
    {
        printf("writing to %s\n",sDselected.toUtf8().constData());
        ofs_D_selected << D_selected << '\n';
    }
    else
        printf("Not writing to %s\n",sDselected.toUtf8().constData());
    ofs_D_selected.close();

    // #### R calculation end ####















    /////////////////////////////////// #5 METHOD I /////////////////////////////////////
    // I) M_D -> I_L1
//    {
//        qDebug() << "METHOD I_L1";
//    }


    /////////////////////////////////// #6 METHOD II /////////////////////////////////////
    // II) D_G^T M -> II_L1
//    {
//        qDebug() << "METHOD II_L1";
//    }


    /////////////////////////////////// #7 METHOD III /////////////////////////////////////
    // III) D^T_{M^T} M -> III_L1
//    {
//        qDebug() << "METHOD III_L1";
//    }


    /////////////////////////////////// #8 METHOD IV /////////////////////////////////////
    // IV) M_D -> I_L2
//    {
//        qDebug() << "METHOD I_L2";
//    }


    /////////////////////////////////// #9 METHOD V /////////////////////////////////////
    // V) D^T_G M -> II_L2
//    {
//        qDebug() << "METHOD II_L2";
//    }


    /////////////////////////////////// #10 METHOD VI /////////////////////////////////////
    // VI) D^T_{M^T} M -> III_L2
//    {
//        qDebug() << "METHOD III_L2";
//    }


    /////////////////////////////////// #11 METHOD VII /////////////////////////////////////
    // VII) M -> IV
//    {
//        qDebug() << "METHOD IV";
//    }






















//    //Condition Numbers Attention - Condition only with fixed orientation!!!!!!!!!
//    VectorXd s;

//    double t_dConditionNumber = MNEMath::getConditionNumber(t_Fwd.sol->data, s);
//    std::cout << "Condition Number:\n" << t_dConditionNumber << std::endl;

//    //
//    QString sKappa = sTargetDir + sTargetPrefix + QString("Kappa.txt");
//    std::ofstream ofs_Kappa(sKappa.toUtf8().constData(), std::ofstream::out);
//    if (ofs_Kappa.is_open())
//    {
//        printf("writing to %s\n",sKappa.toUtf8().constData());
//        ofs_Kappa << t_dConditionNumber << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sKappa.toUtf8().constData());
//    ofs_Kappa.close();

//    // s
//    QString sS = sTargetDir + sTargetPrefix + QString("S.txt");
//    std::ofstream ofs_sS(sS.toUtf8().constData(), std::ofstream::out);
//    if (ofs_sS.is_open())
//    {
//        printf("writing to %s\n",sS.toUtf8().constData());
//        ofs_sS << s << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sS.toUtf8().constData());
//    ofs_sS.close();



//    double t_dConditionNumberClustered_L1 = MNEMath::getConditionNumber(t_clusteredFwd_L1.sol->data, s);
//    std::cout << "Clustered L1 Condition Number:\n" << t_dConditionNumberClustered_L1 << std::endl;

//    //
//    QString sKappaL1 = sTargetDir + sTargetPrefix + QString("Kappa_L1.txt");
//    std::ofstream ofs_KappaL1(sKappaL1.toUtf8().constData(), std::ofstream::out);
//    if (ofs_KappaL1.is_open())
//    {
//        printf("writing to %s\n",sKappaL1.toUtf8().constData());
//        ofs_KappaL1 << t_dConditionNumberClustered_L1 << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sKappaL1.toUtf8().constData());
//    ofs_KappaL1.close();

//    // s
//    QString sSL1 = sTargetDir + sTargetPrefix + QString("S_L1.txt");
//    std::ofstream ofs_SL1(sSL1.toUtf8().constData(), std::ofstream::out);
//    if (ofs_SL1.is_open())
//    {
//        printf("writing to %s\n",sSL1.toUtf8().constData());
//        ofs_SL1 << s << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sSL1.toUtf8().constData());
//    ofs_SL1.close();



//    double t_dConditionNumberClustered_L2 = MNEMath::getConditionNumber(t_clusteredFwd_L2.sol->data, s);
//    std::cout << "Clustered L2 Condition Number:\n" << t_dConditionNumberClustered_L2 << std::endl;

//    // Kappa
//    QString sKappaL2 = sTargetDir + sTargetPrefix + QString("Kappa_L2.txt");
//    std::ofstream ofs_KappaL2(sKappaL2.toUtf8().constData(), std::ofstream::out);
//    if (ofs_KappaL2.is_open())
//    {
//        printf("writing to %s\n",sKappaL2.toUtf8().constData());
//        ofs_KappaL2 << t_dConditionNumberClustered_L2 << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sKappaL2.toUtf8().constData());
//    ofs_KappaL2.close();

//    // s
//    QString sSL2 = sTargetDir + sTargetPrefix + QString("S_L2.txt");
//    std::ofstream ofs_SL2(sSL2.toUtf8().constData(), std::ofstream::out);
//    if (ofs_SL2.is_open())
//    {
//        printf("writing to %s\n",sSL2.toUtf8().constData());
//        ofs_SL2 << s << '\n';
//    }
//    else
//        printf("Not writing to %s\n",sSL2.toUtf8().constData());
//    ofs_SL2.close();


//    double t_dConditionNumberMags = MNEMath::getConditionNumber(mags, s);
//    double t_dConditionNumberMagsClustered = MNEMath::getConditionNumber(magsClustered, s);

//    std::cout << "Condition Number Magnetometers:\n" << t_dConditionNumberMags << std::endl;
//    std::cout << "Clustered Condition Number Magnetometers:\n" << t_dConditionNumberMagsClustered << std::endl;

//    double t_dConditionNumberGrads = MNEMath::getConditionNumber(grads, s);
//    double t_dConditionNumberGradsClustered = MNEMath::getConditionNumber(gradsClustered, s);

//    std::cout << "Condition Number Gradiometers:\n" << t_dConditionNumberGrads << std::endl;
//    std::cout << "Clustered Condition Number Gradiometers:\n" << t_dConditionNumberGradsClustered << std::endl;


    return CommandLineOk;
}
