//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>
#include <iostream>

//DEBUG fstream
#include <fstream>


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

    QFile t_fileFwdMeeg("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile t_fileFwdEeg("./MNE-sample-data/MEG/sample/sample_audvis-eeg-oct-6-fwd.fif");
    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

    qint32 nave = -1;
    double snr = 3.0;
    double lambda2 = 1.0 / pow(snr, 2);
    bool dSPM = false;
    bool sLORETA = true;

    // Load data
    fiff_int_t setno = 0;
    FiffEvokedDataSet evokedSet(t_fileEvoked, setno);
    MNEForwardSolution t_forwardMeeg(t_fileFwdMeeg, false, true); //OK - inconsistend with mne-python when reading with surf_ori = true

    FiffCov noise_cov(t_fileCov); //OK

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evokedSet.info, 0.05, 0.05, 0.1, true); //OK

    // Restrict forward solution as necessary for MEG
    MNEForwardSolution t_forwardMeg = t_forwardMeeg.pick_types(true, false);
    // Alternatively, you can just load a forward solution that is restricted
    MNEForwardSolution t_forwardEeg(t_fileFwdEeg, false, true);

    // make an M/EEG, MEG-only, and EEG-only inverse operators
    FiffInfo info = evokedSet.info;

    MNEInverseOperator inverse_operator_meeg = MNEInverseOperator::make_inverse_operator(info, t_forwardMeeg, noise_cov, 0.2, 0.8);

//    std::cout << "inverse_operator_meeg.eigen_fields:\n" << inverse_operator_meeg.eigen_fields->data.block(0,0,20,20) << std::endl;
//    std::cout << "inverse_operator_meeg.eigen_leads:\n" << inverse_operator_meeg.eigen_leads->data.block(0,0,20,20) << std::endl;

//    MNEInverseOperator inverse_operator_meg = MNEInverseOperator::make_inverse_operator(info, t_forwardMeg, noise_cov, 0.2, 0.8);
//    MNEInverseOperator inverse_operator_eeg = MNEInverseOperator::make_inverse_operator(info, t_forwardEeg, noise_cov, 0.2, 0.8);

    //ToDo create something similiar to mne-pythons "apply_inverse" -> instead create algorithm interface which is consistend between inverse algorithms -> MNE, RAP MUSIC, ...

/*
    //
    //   Set up the inverse according to the parameters
    //
    if (nave < 0)
        nave = evokedSet.evoked[0]->nave;

    MNEInverseOperator inv = inverse_operator_meeg.prepare_inverse_operator(nave,lambda2,dSPM,sLORETA);
    //
    //   Pick the correct channels from the data
    //
    FiffEvokedDataSet newEvokedSet = evokedSet.pick_channels(inv.noise_cov->names);

    evokedSet = newEvokedSet;

    printf("Picked %d channels from the data\n",evokedSet.info.nchan);
    printf("Computing inverse...");
    //
    //   Simple matrix multiplication followed by combination of the
    //   three current components
    //
    //   This does all the data transformations to compute the weights for the
    //   eigenleads
    //
    SparseMatrix<double> reginv(inv.reginv.rows(),inv.reginv.rows());

    // put this in the MNE algorithm class derived from inverse algorithm
    //ToDo put this into a function of inv data
    qint32 i;
    for(i = 0; i < inv.reginv.rows(); ++i)
        reginv.insert(i,i) = inv.reginv(i,0);
    MatrixXd trans = reginv*inv.eigen_fields->data*inv.whitener*inv.proj*evokedSet.evoked[0]->epochs;
    //
    //   Transformation into current distributions by weighting the eigenleads
    //   with the weights computed above
    //
    MatrixXd sol;
    if (inv.eigen_leads_weighted)
    {
        //
        //     R^0.5 has been already factored in
        //
        printf("(eigenleads already weighted)...");
        sol = inv.eigen_leads->data*trans;
    }
    else
    {
        //
        //     R^0.5 has to factored in
        //
       printf("(eigenleads need to be weighted)...");

       SparseMatrix<double> sourceCov(inv.source_cov->data.rows(),inv.source_cov->data.rows());
       for(i = 0; i < inv.source_cov->data.rows(); ++i)
           sourceCov.insert(i,i) = sqrt(inv.source_cov->data(i,0));

       sol   = sourceCov*inv.eigen_leads->data*trans;
    }

    if (inv.source_ori == FIFFV_MNE_FREE_ORI)
    {
        printf("combining the current components...");
        MatrixXd sol1(sol.rows()/3,sol.cols());
        for(i = 0; i < sol.cols(); ++i)
        {
            VectorXd* tmp = MNE::combine_xyz(sol.block(0,i,sol.rows(),1));
            sol1.block(0,i,sol.rows()/3,1) = tmp->cwiseSqrt();
            delete tmp;
        }
        sol.resize(sol1.rows(),sol1.cols());
        sol = sol1;
    }
    if (dSPM)
    {
        printf("(dSPM)...");
        sol = inv.noisenorm*sol;
    }
    else if (sLORETA)
    {
        printf("(sLORETA)...");
        sol = inv.noisenorm*sol;
    }
    printf("[done]\n");

    //Results
//    inv;
//    sol2;
    float tmin = ((float)evokedSet.evoked[0]->first) / evokedSet.info.sfreq;
    float tstep = 1/evokedSet.info.sfreq;

    std::cout << std::endl << "part ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << sol.block(0,0,10,10) << std::endl;
    printf("tmin = %f s\n", tmin);
    printf("tstep = %f s\n", tstep);

//*/

    return a.exec();
}
