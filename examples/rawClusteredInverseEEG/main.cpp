//=============================================================================================================
/**
* @file     main.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2012, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of the computation of a rawClusteredInverse with EEG data
*
*/


//*************************************************************************************************************
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
#include <inverse/minimumNorm/minimumnorm.h>

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>

#include <utils/mnemath.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QSet>
#include <QDir>


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
    QApplication a(argc, argv);

    //############################# Data location and configs - Change if necessary ############################################

    //source localization type
    QString method("MNE"); //"MNE" | "dSPM" | "sLORETA"

    //Choose which epoch to take - i.e. for every 4th epoch set to 4
    int averageIterator = 1;

    bool doRawTrialLocalization = false;

    //measurement data location (data location must always has same directory structure - see dropbox folder for more information)
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2013_12_05_Lorenz_Esch_001");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_01_10_Lorenz_Esch_002");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_01_14_Lorenz_Esch_003");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_01_28_Lorenz_Esch_004");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_04_Lorenz_Esch_005");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_05_Uwe_Graichen_006");
    QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_07_Lorenz_Esch_007");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_24_Lorenz_Esch_008");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_25_Christoph_Dinh_009");
    //QString data_location("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_03_17_Lorenz_Esch_010");

    //Forward solution
    //QFile t_fileFwd("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/Lorenz-131212-Duke128-fwd.fif");
    //QFile t_fileFwd("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/Lorenz-140112-Duke128-fwd.fif");
    //QFile t_fileFwd("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/Lorenz-140114-Duke128-fwd.fif");
    //QFile t_fileFwd("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/Lorenz-140123-Duke128-fwd.fif");
    QFile t_fileFwd("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/Lorenz-140128-Duke128-fwd.fif");


    //Surface generated by freesurfer
    SurfaceSet t_surfSet("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/surface/lh.white", "D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/surface/rh.white");

    //AnnotationSet (See Destrieux paper)
    AnnotationSet t_annotationSet("D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/atlas/lh.aparc.a2009s.annot", "D:/Dropbox/Masterarbeit DB/Messdaten/Forward solutions/atlas/rh.aparc.a2009s.annot");

    //////////////////////////////////////////// Interesting start ////////////////////////////////////////////////////////////////////
    //********************************** Involuntary Tapping ***********************************//
    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -2.0f;
//    float tmax = 2.0f;

//    //Filtered 7-14Hz left tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_left_tapping_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_left_tapping_131_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_left_tapping_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_left_tapping_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_right_tapping_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEEG_data_001_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_right_tapping_136_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_right_tapping_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_right_tapping_filtered_7_14_Averaged_");

//    //Filtered 0.7-40Hz left tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_left_tapping_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_left_tapping_131_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_left_tapping_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_left_tapping_filtered_07_40_Averaged_");

//    //Filtered 0.7-40Hz right tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_right_tapping_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_right_tapping_136_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_right_tapping_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_right_tapping_filtered_07_40_Averaged_");

    //////////////////////////////////////////// Interesting end ////////////////////////////////////////////////////////////////////

    //********************************** Voluntary Tapping ***********************************//

//    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -1.0f;
//    float tmax = 1.0f;

//    //Original left tapping - no filtering DC still present
//    QString t_sRawFileNameRel  ("/Original/EEG_data_001_right_tapping_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_right_tapping_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_right_tapping_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_right_tapping_Averaged_");

//    //Filtered 0.6-40Hz left tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_left_tapping_filtered_06_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_left_tapping_filtered_06_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_left_tapping_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_left_tapping_filtered_06_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_left_tapping_filtered_06_40_Averaged_");

//    //Filtered 0.6-40Hz right tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_right_tapping_filtered_06_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_right_tapping_filtered_06_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_right_tapping_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_right_tapping_filtered_06_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_right_tapping_filtered_06_40_Averaged_");

//    //Filtered 7-14Hz left tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_voluntary_left_tapping_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_tapping_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_voluntary_left_tapping_artefact_reduction_-1_1_0.002_71_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_left_tapping_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_voluntary_left_tapping_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_voluntary_right_tapping_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_tapping_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_voluntary_right_tapping_artefact_reduction_-1_1_0.003_69_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_right_tapping_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_voluntary_right_tapping_filtered_7_14_Averaged_");

//    //Filtered 0.7-40Hz left tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_voluntary_left_tapping_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_tapping_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_voluntary_left_tapping_222_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_left_tapping_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_voluntary_left_tapping_filtered_07_40_Averaged_");

//    //Filtered 0.7-40Hz right tapping
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_voluntary_right_tapping_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_tapping_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_voluntary_right_tapping_184_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_right_tapping_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_voluntary_right_tapping_filtered_07_40_Averaged_");

    //********************************** Medianus stimulation **********************************//

//    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -1.0f;
//    float tmax = 1.0f;

//    //Original left medianus stimulation - no filtering DC still present
//    QString t_sRawFileNameRel  ("/Original/EEG_data_002_medianus_left_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_left_240_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_left_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_left_Averaged_");

//    //Original right medianus stimulation - no filtering DC still present
//    QString t_sRawFileNameRel  ("/Original/EEG_data_002_medianus_right_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_right_295_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_right_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_right_Averaged_");

//    //Filtered 0.7-40Hz left medianus stimulation
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_medianus_left_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_left_artefact_reduction_-1_1_0.002_80_raw-eve.fif"); // ("EEG_data_002_medianus_left_raw-eve")
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_left_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_left_filtered_07_40_Averaged_");

//    //Filtered 0.7-40Hz right medianus stimulation
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_medianus_right_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_right_artefact_reduction_-1_1_0.0025_93_raw-eve.fif");    // ("EEG_data_002_medianus_right_raw-eve")
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_right_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_right_filtered_07_40_Averaged_");

//    //Filtered 7-14Hz left medianus stimulation
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_medianus_left_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_left_artefact_reduction_-1_1_0.002_80_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_left_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_left_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right medianus stimulation
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_medianus_right_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_medianus_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_medianus_right_artefact_reduction_-1_1_0.0025_93_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_medianus_right_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_medianus_right_filtered_7_14_Averaged_");

    //*************************** Voluntary opposing finger movement **************************//

//    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -0.3f;
//    float tmax = 0.1f;

//    //Original left opposing finger movement - no filtering DC still present
//    QString t_sRawFileNameRel  ("/Original/EEG_data_001_voluntary_left_opposing_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_base_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_left_opposing_89_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_left_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_left_Averaged_");

//    //Original right opposing finger movement - no filtering DC still present
//    QString t_sRawFileNameRel  ("/Original/EEG_data_001_voluntary_right_opposing_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_base_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_right_opposing_90_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_right_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_right_Averaged_");

//    //Filtered 7-14Hz left voluntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_voluntary_left_opposing_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_opposing_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_left_opposing_154_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_left_opposing_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_left_opposing_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right voluntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_voluntary_right_opposing_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_opposing_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_right_opposing_135_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_right_opposing_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_right_opposing_filtered_7_14_Averaged_");

//    //Filtered 0.7-40Hz left voluntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_voluntary_left_opposing_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_opposing_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_left_opposing_137_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_left_opposing_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_left_opposing_filtered_07_40_Averaged_");

//    //Filtered 0.7-40Hz right voluntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_voluntary_right_opposing_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_voluntary_opposing_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_right_opposing_135_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_voluntary_right_opposing_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_right_opposing_filtered_07_40_Averaged_");


    //*************************** Involuntary opposing finger movement **************************//

    //time read around events - note: trigger point is the pressing of the button
    float tmin = -1.0f;
    float tmax = 1.0f;

//    //Filtered 7-14Hz left involuntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_left_opposing_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_left_opposing_137_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_left_opposing_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_left_opposing_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right involuntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_right_opposing_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_right_opposing_150_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_right_opposing_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_right_opposing_filtered_7_14_Averaged_");

//    //Filtered 0.7-40Hz left involuntary finger opposing
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_left_opposing_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_left_opposing_137_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_left_opposing_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_left_opposing_filtered_07_40_Averaged_");

    //Filtered 0.7-40Hz right involuntary finger opposing
    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_001_involuntary_right_opposing_filtered_07_40_raw.fif");
    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_001_base_filtered_07_40_raw-cov.fif");
    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_involuntary_right_opposing_150_raw-eve.fif");
    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_involuntary_right_opposing_filtered_07_40_Averaged_");
    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_involuntary_right_opposing_filtered_07_40_Averaged_");

    //*************************** Motor imagery **************************//

//    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -1.0f;
//    float tmax = 6.0f;

//    //Filtered 7-14Hz left MI
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_left_MI_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_002_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_left_MI_filtered_100_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_left_MI_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_left_MI_filtered_7_14_Averaged_");

//    //Filtered 7-14Hz right MI
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_right_MI_filtered_7_14_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_002_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_right_MI_filtered_105_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_right_MI_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_right_MI_filtered_7_14_Averaged_");

//    //Filtered 0.7-40Hz left MI
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_left_MI_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_002_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_left_MI_filtered_100_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_left_MI_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_left_MI_filtered_07_40_Averaged_");

//    //Filtered 0.7-40Hz right MI
//    QString t_sRawFileNameRel  ("/Processed/filtered/EEG_data_002_right_MI_filtered_07_40_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_002_base_filtered_07_40_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_002_right_MI_filtered_105_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_right_MI_filtered_07_40_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_002_right_MI_filtered_07_40_Averaged_");

    //*************************** BCI lefti right in one file finger tapping **************************//

//    //time read around events - note: trigger point is the pressing of the button, not the atual
//    float tmin = -1.0f;
//    float tmax = 1.0f;

//    //Filtered 7-14Hz left MI
//    QString t_sRawFileNameRel  ("/Original/EEG_data_001_voluntary_left_right_tapping_raw.fif");
//    QString t_sCovFileNameRel  ("/Processed/covariance/EEG_data_002_base_filtered_7_14_raw-cov.fif");
//    QString t_sEventFileNameRel("/Processed/events/EEG_data_001_voluntary_left_right_tapping_234_raw-eve.fif");
//    QString t_sStcFileNameRel  ("/Processed/stc/SourceLoc_left_MI_filtered_7_14_Averaged_");
//    QString t_sAvrFileNameRel  ("/Processed/averaged/EEG_data_001_voluntary_right_tapping_raw_Averaged_");

    //##########################################################################################################################

    //Create final location dirs and files
    QFile t_fileRaw(t_sRawFileNameRel.prepend(data_location));
    QFile t_fileCov(t_sCovFileNameRel.prepend(data_location));
    QString t_sEventName = t_sEventFileNameRel.prepend(data_location);

    qint32 event = 1; //254-right trigger | 253-left trigger | 252-beep | 1-old eventfile value for trigger

    qint32 k, p;
    fiff_int_t dest_comp = 0;
    bool keep_comp = false;
    bool pick_all  = false;

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
        //include << "STI 014";
        bool want_meg   = false;
        bool want_eeg   = true;
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

            data.append(MNEEpochData::SPtr(epoch)); //List takes ownwership of the pointer - no delete need
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

    //Only take first finger movement of each tapping session (each tapping session consists of 4 seperate finger movement)
//    VectorXi vecSel(1);
//    vecSel << 60;

    if(count<averageIterator)
        averageIterator = count;

    VectorXi vecSel(count/averageIterator);

    int i;
    for(i=0; i<count/averageIterator; i++)
        vecSel[i] = i*averageIterator;

    // Add number of averages read from the eventfile to the file name
    t_sAvrFileNameRel.append(t_sAvrFileNameRel.number(tmin));
    t_sAvrFileNameRel.append("_");
    t_sAvrFileNameRel.append(t_sAvrFileNameRel.number(tmax));
    t_sAvrFileNameRel.append("_");
    t_sAvrFileNameRel.append(t_sAvrFileNameRel.number(i));
    t_sAvrFileNameRel.append(".fif");
    QString t_sAvrFileName = t_sAvrFileNameRel.prepend(data_location);

    t_sStcFileNameRel.append(t_sStcFileNameRel.number(tmin));
    t_sStcFileNameRel.append("_");
    t_sStcFileNameRel.append(t_sStcFileNameRel.number(tmax));
    t_sStcFileNameRel.append("_");
    t_sStcFileNameRel.append(t_sStcFileNameRel.number(i));
    t_sStcFileNameRel.append("_");
    t_sStcFileNameRel.append(method.append(".stc"));
    QString t_sFileNameStc = t_sStcFileNameRel.prepend(data_location);

    std::cout << "Select following epochs to average:\n" << vecSel << std::endl;
    raw.info.filename = QString(""); //this need to be done in order to write a new file. otherwise some of the info contents are not getting copied correctly from raw.info (i.e. digitizer data)
    FiffEvoked evoked = data.average(raw.info, tmin*raw.info.sfreq, floor(tmax*raw.info.sfreq + 0.5), vecSel);

    //Write averaged data to file
    QFile m_fileOut(t_sAvrFileName);
    RowVectorXd m_cals;

    FiffStream::SPtr m_pOutfid = Fiff::start_writing_raw(m_fileOut, raw.info, m_cals);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
    //m_pOutfid->finish_writing_raw();

    Eigen::MatrixXd eData = evoked.data;
    Eigen::MatrixXd matValue = MatrixXd::Zero(eData.rows()+10, eData.cols());

    matValue.block(0,0,128,matValue.cols()) = evoked.data;

    m_pOutfid->write_raw_buffer(matValue, m_cals);

    m_pOutfid->finish_writing_raw();

    //################################# Source Estimate start ##########################################

    double snr = 0.1f;//1.0f;//3.0f;//0.1f;//3.0f;

    QString t_sFileNameClusteredInv("");

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

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40); % Use multithreading
    //MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 10);//40);

    //MNEForwardSolution t_clusteredFwd = t_Fwd;

    //
    // Find rows of interest in stc files
    //

    QFile wrtFWD ("./mne_scan_plugins/resources/tmsi/fwd_clustered.txt");
    wrtFWD.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&wrtFWD);

    //Read vertnos
    VectorXi vertno_left = t_clusteredFwd.src[0].vertno;
    VectorXi vertno_right = t_clusteredFwd.src[1].vertno;

    out<<"Vertno Left Hemi:"<<endl<<endl;
    for(int i=0; i<vertno_left.rows(); i++)
        out<<vertno_left[i]<<endl;

    out<<endl<<"Vertno right Hemi:"<<endl<<endl;
    for(int i=0; i<vertno_right.rows(); i++)
        out<<vertno_right[i]<<endl;

    //Read corresponding labels
    VectorXi labelIds_left = t_annotationSet[0].getLabelIds();
    out<<endl<<endl<<"labelIds_left:"<<endl<<endl;
    for(int i=0; i<labelIds_left.rows(); i++)
        out<<labelIds_left[i]<<endl;

    VectorXi labelIds_right = t_annotationSet[1].getLabelIds();
    out<<endl<<endl<<"labelIds_right:"<<endl<<endl;
    for(int i=0; i<labelIds_right.rows(); i++)
        out<<labelIds_right[i]<<endl;

    //Find interesting rows in stc files
    // Left hemisphere
    QFile wrtFWD_28_left ("./mne_scan_plugins/resources/tmsi/fwd_clustered_postcentral_28_Left.txt");
    wrtFWD_28_left.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_28_left(&wrtFWD_28_left);

    QFile wrtFWD_29_left ("./mne_scan_plugins/resources/tmsi/fwd_clustered_precentral_29_Left.txt");
    wrtFWD_29_left.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_29_left(&wrtFWD_29_left);

    QFile wrtFWD_45_left ("./mne_scan_plugins/resources/tmsi/fwd_clustered_central_45_Left.txt");
    wrtFWD_45_left.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_45_left(&wrtFWD_45_left);

    QVector<int> interestingRows_Left;
    out<<endl<<endl<<"interestingRows_Left (matlab syntax):"<<endl<<endl;
    for(int i=0; i<vertno_left.rows() ; i++)
    {
        //G_postcentral
        if(labelIds_left[vertno_left[i]] == 9221140)
        {
            interestingRows_Left.push_back(i);
            out<<"Region 28 - G_postcentral: "<<i+1<<endl;
            out_28_left<<i+1<<endl;
        }

        //G_precentral
        if(labelIds_left[vertno_left[i]] == 11832380)
        {
            interestingRows_Left.push_back(i);
            out<<"Region 29 - G_precentral: "<<i+1<<endl;
            out_29_left<<i+1<<endl;
        }

        //S_central
        if(labelIds_left[vertno_left[i]] == 660701)
        {
            interestingRows_Left.push_back(i);
            out<<"Region 45 - S_central: "<<i+1<<endl;
            out_45_left<<i+1<<endl;
        }
    }

    // Right hemisphere
    QFile wrtFWD_28_right ("./mne_scan_plugins/resources/tmsi/fwd_clustered_postcentral_28_Right.txt");
    wrtFWD_28_right.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_28_right(&wrtFWD_28_right);

    QFile wrtFWD_29_right ("./mne_scan_plugins/resources/tmsi/fwd_clustered_precentral_29_Right.txt");
    wrtFWD_29_right.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_29_right(&wrtFWD_29_right);

    QFile wrtFWD_45_right ("./mne_scan_plugins/resources/tmsi/fwd_clustered_central_45_Right.txt");
    wrtFWD_45_right.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out_45_right(&wrtFWD_45_right);

    QVector<int> interestingRows_Right;
    out<<endl<<endl<<"interestingRows_Right (matlab syntax):"<<endl<<endl;
    for(int i=0; i<vertno_right.rows() ; i++)
    {
        //G_postcentral
        if(labelIds_right[vertno_right[i]] == 9221140)
        {
            interestingRows_Right.push_back(i);
            out<<"Region 28 - G_postcentral: "<<i+1+vertno_left.rows()<<endl;
            out_28_right<<i+1<<endl;
        }

        //G_precentral
        if(labelIds_right[vertno_right[i]] == 11832380)
        {
            interestingRows_Right.push_back(i);
            out<<"Region 29 - G_precentral: "<<i+1+vertno_left.rows()<<endl;
            out_29_right<<i+1<<endl;
        }

        //S_central
        if(labelIds_right[vertno_right[i]] == 660701)
        {
            interestingRows_Right.push_back(i);
            out<<"Region 45 - S_central: "<<i+1+vertno_left.rows()<<endl;
            out_45_right<<i+1<<endl;
        }
    }

    wrtFWD_28_left.close();
    wrtFWD_29_left.close();
    wrtFWD_45_left.close();
    wrtFWD_28_right.close();
    wrtFWD_29_right.close();
    wrtFWD_45_right.close();
    wrtFWD.close();

    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

//    QFile t_fileInv("D:/Dropbox/Masterarbeit DB/Messdaten/EEG/2014_02_07_Lorenz_Esch_007/Processed/inverse operators/Lorenz-140128-Duke128-eeg-inv.fif");
//    MNEInverseOperator inverse_operator(t_fileInv);

    //
    // save clustered inverse
    //
    if(!t_sFileNameClusteredInv.isEmpty())
    {
        QFile t_fileClusteredInverse(t_sFileNameClusteredInv);
        inverse_operator.write(t_fileClusteredInverse);
    }

    //
    // Compute inverse solution
    //
    // Calculate stc for averaged data
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);

    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    if(sourceEstimate.isEmpty())
        return 1;

    // Calculate stc for all trials and write to file - dirty
    if(doRawTrialLocalization)
    {
        MNESourceEstimate sourceEstimate_trial;

        for(int i = 0; i<data.size(); i++)
        {
            QString file_trial_stc_orig = t_sFileNameStc;

            QString file_trial_stc_tmp = t_sFileNameStc;
            QString stc_trial_sub_folder = file_trial_stc_tmp.remove(0,t_sFileNameStc.indexOf("/So")+1);

            stc_trial_sub_folder.remove(".stc");
            stc_trial_sub_folder.append("-raw-trials/");

            QString dirCheck = data_location;
            dirCheck.append("/Processed/stc/");
            dirCheck.append(stc_trial_sub_folder);

            if(QDir(dirCheck).exists() == false)
                QDir().mkdir(dirCheck);

            stc_trial_sub_folder.prepend("stc/");

            file_trial_stc_orig.replace("stc/", stc_trial_sub_folder);

            QString temp;
            temp.append("_");
            temp.append(temp.number(i));
            temp.append(".stc");

            file_trial_stc_orig.replace(".stc",temp);

            sourceEstimate_trial = minimumNorm.calculateInverse(data[i]->epoch,0,1/info.sfreq);

            if(sourceEstimate_trial.isEmpty())
                return 1;

            if(!t_sFileNameStc.isEmpty())
            {
                QFile t_fileTrialStc(file_trial_stc_orig);
                sourceEstimate_trial.write(t_fileTrialStc);
            }
        }
    }

    // View activation time-series
    std::cout << "\nsourceEstimate:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimate.times.block(0,0,1,10) << std::endl;
    std::cout << "timeMin\n" << sourceEstimate.times[0] << std::endl;
    std::cout << "timeMax\n" << sourceEstimate.times[sourceEstimate.times.size()-1] << std::endl;
    std::cout << "time step\n" << sourceEstimate.tstep << std::endl;

    VectorXd s;

    //double t_dConditionNumber = MNEMath::getConditionNumber(t_Fwd.sol->data, s);
    //double t_dConditionNumberClustered = MNEMath::getConditionNumber(t_clusteredFwd.sol->data, s);

//    std::cout << "Condition Number:\n" << t_dConditionNumber << std::endl;
//    std::cout << "Clustered Condition Number:\n" << t_dConditionNumberClustered << std::endl;

//    std::cout << "ForwardSolution" << t_Fwd.sol->data.block(0,0,10,10) << std::endl;

    std::cout << "Clustered ForwardSolution" << t_clusteredFwd.sol->data.block(0,0,10,10) << std::endl;

    // Write stc to file
    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        sourceEstimate.write(t_fileClusteredStc);
    }

    //############################### Source Estimate end #########################################

    //only one time point - P100
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

//   sourceEstimate = sourceEstimate.reduce(253, 1); //1731

    View3D::SPtr testWindow = View3D::SPtr(new View3D());
    testWindow->addSurfaceSet("Subject01", "HemiLRSet", t_surfSet, t_annotationSet);

    QList<BrainRTSourceLocDataTreeItem*> rtItemList = testWindow->addSourceData("Subject01", "HemiLRSet", sourceEstimate, t_clusteredFwd);

    testWindow->show();

    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->setView3D(testWindow);
    control3DWidget->show();

    return a.exec();//1;//a.exec();
}
