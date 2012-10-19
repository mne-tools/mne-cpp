//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
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
* @brief    Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../../../MNE/fiff/fiff.h"
#include "../../../MNE/mne/mne.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;


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
    QCoreApplication a(argc, argv);

//  fname_data  - Name of the data file
//  setno       - Data set number
//  fname_inv   - Inverse operator file name
//  nave        - Number of averages (scales the noise covariance)
//             If negative, the number of averages in the data will be
//             used
//  lambda2     - The regularization factor
//  dSPM        - do dSPM?
//  sLORETA     - do sLORETA?
    QString t_sFileEvoked = "../../mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis-ave.fif";
    qint32 setno = 0;
    QString t_sFileInv = "../../mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    qint32 nave = 40;
    float snr = 3.0f;
    float lambda2 = pow(1.0f / snr, 2.0f);
    bool dSPM = false;
    bool sLORETA = true;

    //
    //   Read the data first
    //
    FiffEvokedDataSet* data = NULL;
//    data = fiff_read_evoked(fname_data,setno);
    Fiff::read_evoked(t_sFileEvoked, data, setno);

    //
    //   Then the inverse operator
    //
    MNEInverseOperator* inv = NULL;
    MNE::read_inverse_operator(t_sFileInv, inv);
    //
    //   Set up the inverse according to the parameters
    //
    if (nave < 0)
        nave = data->evoked->nave;
//    inv =
    MNE::prepare_inverse_operator(inv,nave,lambda2,dSPM,sLORETA);
    //
    //   Pick the correct channels from the data
    //
//    data = fiff_pick_channels_evoked(data,inv.noise_cov.names);
//    fprintf(1,'Picked %d channels from the data\n',data.info.nchan);
//    fprintf(1,'Computing inverse...');
//    %
//    %   Simple matrix multiplication followed by combination of the
//    %   three current components
//    %
//    %   This does all the data transformations to compute the weights for the
//    %   eigenleads
//    %
//    trans = diag(sparse(inv.reginv))*inv.eigen_fields.data*inv.whitener*inv.proj*double(data.evoked(1).epochs);
//    %
//    %   Transformation into current distributions by weighting the eigenleads
//    %   with the weights computed above
//    %
//    if inv.eigen_leads_weighted
//       %
//       %     R^0.5 has been already factored in
//       %
//       fprintf(1,'(eigenleads already weighted)...');
//       sol   = inv.eigen_leads.data*trans;
//    else
//       %
//       %     R^0.5 has to factored in
//       %
//       fprintf(1,'(eigenleads need to be weighted)...');
//       sol   = diag(sparse(sqrt(inv.source_cov.data)))*inv.eigen_leads.data*trans;
//    end

//    if inv.source_ori == FIFF.FIFFV_MNE_FREE_ORI
//        fprintf(1,'combining the current components...');
//        sol1 = zeros(size(sol,1)/3,size(sol,2));
//        for k = 1:size(sol,2)
//            sol1(:,k) = sqrt(mne_combine_xyz(sol(:,k)));
//        end
//        sol = sol1;
//    end
//    if dSPM
//        fprintf(1,'(dSPM)...');
//        sol = inv.noisenorm*sol;
//    elseif sLORETA
//        fprintf(1,'(sLORETA)...');
//        sol = inv.noisenorm*sol;
//    end
//    res.inv   = inv;
//    res.sol   = sol;
//    res.tmin  = double(data.evoked(1).first)/data.info.sfreq;
//    res.tstep = 1/data.info.sfreq;
//    printf("[done]\n");























    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
