//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
* @brief    Example of the computation of a single trial clustered inverse rap music
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
#include <inverse/rapMusic/pwlrapmusic.h>

#include <disp3D/inverseview.h>

#include <utils/mnemath.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGuiApplication>
#include <QSet>


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

//    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
//    QFile t_fileFwd("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
//    AnnotationSet t_annotationSet("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot", "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot");
//    SurfaceSet t_surfSet("./MNE-sample-data/subjects/sample/surf/lh.white", "./MNE-sample-data/subjects/sample/surf/rh.white");

    QFile t_fileRaw("E:/Data/sl_data/MEG/mind006/mind006_051209_auditory01_raw.fif");
    QFile t_fileFwd("E:/Data/sl_data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-fwd.fif");
    AnnotationSet t_annotationSet("E:/Data/sl_data/subjects/mind006/label/lh.aparc.a2009s.annot", "E:/Data/sl_data/subjects/mind006/label/rh.aparc.a2009s.annot");
    SurfaceSet t_surfSet("E:/Data/sl_data/subjects/mind006/surf/lh.white", "E:/Data/sl_data/subjects/mind006/surf/rh.white");

//    QFile t_fileRaw("E:/Data/sl_data/MEG/mind006/mind006_051209_median01_raw.fif");
//    QFile t_fileFwd("E:/Data/sl_data/MEG/mind006/mind006_051209_median01_raw-oct-6-fwd.fif");
//    AnnotationSet t_annotationSet("E:/Data/sl_data/subjects/mind006/label/lh.aparc.a2009s.annot", "E:/Data/sl_data/subjects/mind006/label/rh.aparc.a2009s.annot");
//    SurfaceSet t_surfSet("E:/Data/sl_data/subjects/mind006/surf/lh.white", "E:/Data/sl_data/subjects/mind006/surf/rh.white");

    QString t_sFileNameStc("");//("mind006_051209_auditory01.stc");

    qint32 numDipolePairs = 7;

    qint32 startSample = 0;
    qint32 numSample = 10000;

    bool in_samples = true;
    bool keep_comp = true;

    // Parse command line parameters
    for(qint32 i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-stc") == 0 || strcmp(argv[i], "--stc") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameStc = QString::fromUtf8(argv[i+1]);
        }
    }

    //
    // Read raw data
    //
    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    float from = raw.first_samp + startSample;
    float to = from + numSample - 1;

    //
    //   Set up pick list: MEG + STI 014 - bad channels
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
    //   Set up the CTF compensator
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
            return -1;
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

    //########################################################################################
    // RAP MUSIC Source Estimate

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution_ccr(t_annotationSet, 20);//40);

    //
    // Compute inverse solution
    //
    PwlRapMusic t_pwlRapMusic(t_clusteredFwd, false, numDipolePairs);

    float tstep = 1.0f/raw.info.sfreq;

    MNESourceEstimate sourceEstimate = t_pwlRapMusic.calculateInverse(data, 0.0f, tstep);
    if(sourceEstimate.isEmpty())
        return 1;

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






//    QList<Label> t_qListLabels;
//    QList<RowVector4i> t_qListRGBAs;

//    //ToDo overload toLabels using instead of t_surfSet rr of MNESourceSpace
//    t_annotationSet.toLabels(t_surfSet, t_qListLabels, t_qListRGBAs);

//    InverseView view(t_pwlRapMusic.getSourceSpace(), t_qListLabels, t_qListRGBAs, 24, true, false, false);//true);

//    if (view.stereoType() != QGLView::RedCyanAnaglyph)
//        view.camera()->setEyeSeparation(0.3f);
//    QStringList args = QCoreApplication::arguments();
//    int w_pos = args.indexOf("-width");
//    int h_pos = args.indexOf("-height");
//    if (w_pos >= 0 && h_pos >= 0)
//    {
//        bool ok = true;
//        int w = args.at(w_pos + 1).toInt(&ok);
//        if (!ok)
//        {
//            qWarning() << "Could not parse width argument:" << args;
//            return 1;
//        }
//        int h = args.at(h_pos + 1).toInt(&ok);
//        if (!ok)
//        {
//            qWarning() << "Could not parse height argument:" << args;
//            return 1;
//        }
//        view.resize(w, h);
//    }
//    else
//    {
//        view.resize(800, 600);
//    }
//    view.show();

//    //Push Estimate
//    view.pushSourceEstimate(sourceEstimate);

//    if(!t_sFileNameStc.isEmpty())
//    {
//        QFile t_fileClusteredStc(t_sFileNameStc);
//        sourceEstimate.write(t_fileClusteredStc);
//    }

    return a.exec();//1;//a.exec();
}
