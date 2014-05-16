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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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
    QGuiApplication a(argc, argv);

//    QFile t_fileFwd("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
//    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
//    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
//    QString t_sEventName = "./MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif";
//    AnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");
//    SurfaceSet t_surfSet("sample", 2, "white", "./MNE-sample-data/subjects");

    QFile t_fileFwd("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-fwd.fif");
    QFile t_fileCov("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-cov.fif");
    QFile t_fileRaw("D:/Data/MEG/mind006/mind006_051209_auditory01_raw.fif");
    QString t_sEventName = "D:/Data/MEG/mind006/mind006_051209_auditory01_raw-eve.fif";
    AnnotationSet t_annotationSet("mind006", 2, "aparc.a2009s", "D:/Data/subjects");
    SurfaceSet t_surfSet("mind006", 2, "white", "D:/Data/subjects");

    qint32 event = 1;

    float tmin = -0.2f;
    float tmax = 0.4f;

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;
    bool pick_all  = true;

    qint32 k, p;

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

//        picks = Fiff::pick_types(raw.info, want_meg, want_eeg, want_stim, include, raw.info.bads);
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
//    qint32 current_comp = MNE::get_current_comp(raw.info);
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
//            raw.info.chs = MNE::set_current_comp(raw.info.chs,dest_comp);
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

    //
    // calculate the average
    //
//    //Option 1
//    qint32 numAverages = 99;
//    VectorXi vecSel(numAverages);
//    srand (time(NULL)); // initialize random seed

//    for(qint32 i = 0; i < vecSel.size(); ++i)
//    {
//        qint32 val = rand() % data.size();
//        vecSel(i) = val;
//    }

//    //Option 2
//    VectorXi vecSel(20);

////    vecSel << 76, 74, 13, 61, 97, 94, 75, 71, 60, 56, 26, 57, 56, 0, 52, 72, 33, 86, 96, 67;

//    vecSel << 65, 22, 47, 55, 16, 29, 14, 36, 57, 97, 89, 46, 9, 93, 83, 52, 71, 52, 3, 96;

    //Option 3 Newest
    VectorXi vecSel(10);

    vecSel << 0, 96, 80, 55, 66, 25, 26, 2, 55, 58, 6, 88;


    std::cout << "Select following epochs to average:\n" << vecSel << std::endl;

    FiffEvoked evoked = data.average(raw.info, tmin*raw.info.sfreq, floor(tmax*raw.info.sfreq + 0.5), vecSel);



    //########################################################################################
    // Source Estimate

    double snr = 1.0f;//0.1f;//1.0f;//3.0f;//0.1f;//3.0f;
    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

    QString t_sFileNameClusteredInv("");
    QString t_sFileNameStc("mind006_051209_auditory01.stc");

    // Parse command line parameters
    for(qint32 i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-snr") == 0 || strcmp(argv[i], "--snr") == 0)
        {
            if(i + 1 < argc)
                snr = atof(argv[i+1]);
        }
        else if(strcmp(argv[i], "-method") == 0 || strcmp(argv[i], "--method") == 0)
        {
            if(i + 1 < argc)
                method = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-inv") == 0 || strcmp(argv[i], "--inv") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameClusteredInv = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-stc") == 0 || strcmp(argv[i], "--stc") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameStc = QString::fromUtf8(argv[i+1]);
        }
    }

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

//    // Load data
//    fiff_int_t setno = 1;
//    QPair<QVariant, QVariant> baseline(QVariant(), 0);
//    FiffEvoked evoked(t_fileEvoked, setno, baseline);
//    if(evoked.isEmpty())
//        return 1;


    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MatrixXd D;
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20, D, noise_cov, evoked.info);

    t_clusteredFwd.src[0].cluster_info.write("ClusterInfoLH.txt");
    t_clusteredFwd.src[1].cluster_info.write("ClusterInfoRH.txt");

    std::cout << "D " << D.rows() << " x " << D.cols() << std::endl;

    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;




    QFile t_fileSelectedFwd("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-5-fwd.fif");

    MNEForwardSolution t_selectedRawFwd(t_fileSelectedFwd);
    if(t_selectedRawFwd.isEmpty())
        return 1;

    MatrixXd D_selected;
    MNEForwardSolution t_selectedFwd = t_selectedRawFwd.reduce_forward_solution(t_clusteredFwd.isFixedOrient() ? t_clusteredFwd.sol->data.cols() : t_clusteredFwd.sol->data.cols()/3, D_selected);

    qDebug() << "#### t_selectedFwd" << t_selectedFwd.sol->data.rows() << "x" << t_selectedFwd.sol->data.cols();

    MNEInverseOperator inverse_operator_selected(info, t_selectedFwd, noise_cov, 0.2f, 0.8f);

    qDebug() << "#### [1] ####";

    MNEInverseOperator inverse_operator_clustered(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

    qDebug() << "#### [2] ####";

    MNEInverseOperator inverse_operator(info, t_Fwd, noise_cov, 0.2f, 0.8f);

    qDebug() << "#### [3] ####";

    //
    // save clustered inverse
    //
    if(!t_sFileNameClusteredInv.isEmpty())
    {
        QFile t_fileClusteredInverse(t_sFileNameClusteredInv);
        inverse_operator_clustered.write(t_fileClusteredInverse);
    }

    //
    // Compute inverse solution
    //

    MinimumNorm minimumNormSelected(inverse_operator_selected, lambda2, method);
    MinimumNorm minimumNormClustered(inverse_operator_clustered, lambda2, method);
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);


#ifdef BENCHMARK
    //
    //   Set up the inverse according to the parameters
    //
    minimumNormClustered.doInverseSetup(vecSel.size(),false);

    MNESourceEstimate sourceEstimate;
    QList<qint64> qVecElapsedTime;
    for(qint32 i = 0; i < 100; ++i)
    {
        //Benchmark time
        QElapsedTimer timer;
        timer.start();
        sourceEstimate = minimumNormClustered.calculateInverse(evoked.data, evoked.times(0), evoked.times(1)-evoked.times(0));
        qVecElapsedTime.append(timer.elapsed());
    }

    double meanTime = 0.0;
    qint32 offset = 19;
    qint32 c = 0;
    for(qint32 i = offset; i < qVecElapsedTime.size(); ++i)
    {
        meanTime += qVecElapsedTime[i];
        ++c;
    }

    meanTime /= (double)c;

    double varTime = 0;
    for(qint32 i = offset; i < qVecElapsedTime.size(); ++i)
        varTime += pow(qVecElapsedTime[i] - meanTime,2);

    varTime /= (double)c - 1.0f;
    varTime = sqrt(varTime);

    qDebug() << "MNE calculation took" << meanTime << "+-" << varTime << "ms in average";

#else
    MNESourceEstimate sourceEstimateSelected = minimumNormSelected.calculateInverse(evoked);
    MNESourceEstimate sourceEstimateClustered = minimumNormClustered.calculateInverse(evoked);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);
#endif


    qDebug() << "#### [4] ####";


    //Option c
//    qDebug() << "Cluster Kernel";
//    MatrixXd D_MT;
//    MatrixXd MT_clustered = minimumNorm.getPreparedInverseOperator().cluster_kernel(t_annotationSet, 20, D_MT);

//    qDebug() << "Cluster Kernel" << MT_clustered.rows() << "x" << MT_clustered.cols();

//    qDebug() << "D_MT" << D_MT.rows() << "x" << D_MT.cols();

    // #### R calculation ####
    QFile t_fileFwdFixed("D:/Data/MEG/mind006/mind006_051209_auditory01_raw-oct-6-fwd-fixed.fif");
    MNEForwardSolution t_FwdFixed(t_fileFwdFixed);
    if(t_FwdFixed.isEmpty())
        return 1;
//    qDebug() << "t_FwdFixed" << t_FwdFixed.sol->data.rows() << "x" << t_FwdFixed.sol->data.cols();

//    //Option b)
//    printf("[1]\n");
//    MatrixXd M = D.transpose() * minimumNorm.getKernel();

//    printf("[2]\n");
//    MatrixXd R = M * t_FwdFixed.sol->data;

//    std::ofstream ofs_R("R_ec.txt", std::ofstream::out);
//    if (ofs_R.is_open())
//    {
//        printf("writing to R_ec.txt\n");
//        ofs_R << R << '\n';
//    }
//    else
//        printf("Not writing to R_ec.txt\n");
//    ofs_R.close();

//    M.resize(0,0);
//    R.resize(0,0);

//    //Option a)
//    printf("[3]\n");
//    MatrixXd M_clusterd = minimumNormClustered.getKernel();

//    printf("[4]\n");
//    MatrixXd R_clustered = M_clusterd * t_FwdFixed.sol->data;

//    std::ofstream ofs_R_clustered("R_clustered_ec.txt", std::ofstream::out);
//    if (ofs_R_clustered.is_open())
//    {
//        printf("writing to R_clustered_ec.txt\n");
//        ofs_R_clustered << R_clustered << '\n';
//    }
//    else
//        printf("Not writing to R_clustered_ec.txt\n");
//    ofs_R_clustered.close();

//    M_clusterd.resize(0,0);
//    R_clustered.resize(0,0);

////Cluster Operator D
//    std::ofstream ofs_D("D_ec.txt", std::ofstream::out);
//    if (ofs_D.is_open())
//    {
//        printf("writing to D_ec.txt\n");
//        ofs_D << D << '\n';
//    }
//    else
//        printf("Not writing to D_ec.txt\n");
//    ofs_D.close();


//    //option c)
//    printf("[5]\n");
//    MatrixXd R_MT_clustered = MT_clustered.transpose() * t_FwdFixed.sol->data;

//    std::ofstream ofs_R_MT_clustered("R_MT_clustered_ec.txt", std::ofstream::out);
//    if (ofs_R_MT_clustered.is_open())
//    {
//        printf("writing to R_MT_clustered_ec.txt\n");
//        ofs_R_MT_clustered << R_MT_clustered << '\n';
//    }
//    else
//        printf("Not writing to R_MT_clustered_ec.txt\n");
//    ofs_R_MT_clustered.close();

//    R_MT_clustered.resize(0,0);

//    //Cluster Operator D
//    std::ofstream ofs_D_MT("D_MT_ec.txt", std::ofstream::out);
//    if (ofs_D_MT.is_open())
//    {
//        printf("writing to D_MT_ec.txt\n");
//        ofs_D_MT << D_MT << '\n';
//    }
//    else
//        printf("Not writing to D_MT_ec.txt\n");
//    ofs_D_MT.close();



    //option d)
    printf("[6]\n");
    MatrixXd M_selected = minimumNormSelected.getKernel();

    qDebug() << "M_selected: " << M_selected.rows() << "x" << M_selected.cols();


    printf("[7]\n");
    MatrixXd R_selected= M_selected * t_FwdFixed.sol->data;

    std::ofstream ofs_R_selected("R_selected.txt", std::ofstream::out);
    if (ofs_R_selected.is_open())
    {
        printf("writing to R_selected.txt\n");
        ofs_R_selected << R_selected << '\n';
    }
    else
        printf("Not writing to R_selected.txt\n");
    ofs_R_selected.close();

    R_selected.resize(0,0);

    //Cluster Operator D
    std::ofstream ofs_D_selected("D_selected.txt", std::ofstream::out);
    if (ofs_D_selected.is_open())
    {
        printf("writing to D_selected.txt\n");
        ofs_D_selected << D_selected << '\n';
    }
    else
        printf("Not writing to D_selected.txt\n");
    ofs_D_selected.close();

    // #### R calculation end ####


    if(sourceEstimateClustered.isEmpty())
        return 1;

    // View activation time-series
    std::cout << "\nsourceEstimate:\n" << sourceEstimateClustered.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimateClustered.times.block(0,0,1,10) << std::endl;
    std::cout << "timeMin\n" << sourceEstimateClustered.times[0] << std::endl;
    std::cout << "timeMax\n" << sourceEstimateClustered.times[sourceEstimateClustered.times.size()-1] << std::endl;
    std::cout << "time step\n" << sourceEstimateClustered.tstep << std::endl;

    //Condition Numbers
//    MatrixXd mags(102, t_Fwd.sol->data.cols());
//    qint32 count = 0;
//    for(qint32 i = 2; i < 306; i += 3)
//    {
//        mags.row(count) = t_Fwd.sol->data.row(i);
//        ++count;
//    }
//    MatrixXd magsClustered(102, t_clusteredFwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 2; i < 306; i += 3)
//    {
//        magsClustered.row(count) = t_clusteredFwd.sol->data.row(i);
//        ++count;
//    }

//    MatrixXd grads(204, t_Fwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 0; i < 306; i += 3)
//    {
//        grads.row(count) = t_Fwd.sol->data.row(i);
//        ++count;
//        grads.row(count) = t_Fwd.sol->data.row(i+1);
//        ++count;
//    }
//    MatrixXd gradsClustered(204, t_clusteredFwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 0; i < 306; i += 3)
//    {
//        gradsClustered.row(count) = t_clusteredFwd.sol->data.row(i);
//        ++count;
//        gradsClustered.row(count) = t_clusteredFwd.sol->data.row(i+1);
//        ++count;
//    }

    VectorXd s;

    double t_dConditionNumber = MNEMath::getConditionNumber(t_Fwd.sol->data, s);
    double t_dConditionNumberClustered = MNEMath::getConditionNumber(t_clusteredFwd.sol->data, s);


    std::cout << "Condition Number:\n" << t_dConditionNumber << std::endl;
    std::cout << "Clustered Condition Number:\n" << t_dConditionNumberClustered << std::endl;

    std::cout << "ForwardSolution" << t_Fwd.sol->data.block(0,0,10,10) << std::endl;

    std::cout << "Clustered ForwardSolution" << t_clusteredFwd.sol->data.block(0,0,10,10) << std::endl;


//    double t_dConditionNumberMags = MNEMath::getConditionNumber(mags, s);
//    double t_dConditionNumberMagsClustered = MNEMath::getConditionNumber(magsClustered, s);

//    std::cout << "Condition Number Magnetometers:\n" << t_dConditionNumberMags << std::endl;
//    std::cout << "Clustered Condition Number Magnetometers:\n" << t_dConditionNumberMagsClustered << std::endl;

//    double t_dConditionNumberGrads = MNEMath::getConditionNumber(grads, s);
//    double t_dConditionNumberGradsClustered = MNEMath::getConditionNumber(gradsClustered, s);

//    std::cout << "Condition Number Gradiometers:\n" << t_dConditionNumberGrads << std::endl;
//    std::cout << "Clustered Condition Number Gradiometers:\n" << t_dConditionNumberGradsClustered << std::endl;


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

    QList<Label> t_qListLabels;
    QList<RowVector4i> t_qListRGBAs;

    //ToDo overload toLabels using instead of t_surfSet rr of MNESourceSpace
    t_annotationSet.toLabels(t_surfSet, t_qListLabels, t_qListRGBAs);

    InverseView view(minimumNormClustered.getSourceSpace(), t_qListLabels, t_qListRGBAs, 24, true, false, true);

    if (view.stereoType() != QGLView::RedCyanAnaglyph)
        view.camera()->setEyeSeparation(0.3f);
    QStringList args = QCoreApplication::arguments();
    int w_pos = args.indexOf("-width");
    int h_pos = args.indexOf("-height");
    if (w_pos >= 0 && h_pos >= 0)
    {
        bool ok = true;
        int w = args.at(w_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse width argument:" << args;
            return 1;
        }
        int h = args.at(h_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse height argument:" << args;
            return 1;
        }
        view.resize(w, h);
    }
    else
    {
        view.resize(800, 600);
    }
    view.show();

    //Push Estimate
    view.pushSourceEstimate(sourceEstimateClustered);

    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        sourceEstimateClustered.write(t_fileClusteredStc);
    }

//*/
    return a.exec();//1;//a.exec();
}
