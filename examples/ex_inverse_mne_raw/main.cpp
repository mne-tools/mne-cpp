//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
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
* @brief    Example of a computation of a raw inverse
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

#include <fiff/fiff_evoked_set.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>


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

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Compute Inverse Powell RAP-MUSIC Example");
    parser.addHelpOption();
    QCommandLineOption sampleEvokedFileOption("ave", "Path to evoked <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption invFileOption("inv", "Path to inverse <file>, which is to be loaded.", "file", "");
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "1.0f");//3.0f;//0.1f;//3.0f;
    QCommandLineOption numberAveragesOption("numAve", "The <value> for the number of averages.", "value", "40");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");//"MNE" | "dSPM" | "sLORETA"

    parser.addOption(sampleEvokedFileOption);
    parser.addOption(invFileOption);
    parser.addOption(snrOption);
    parser.addOption(numberAveragesOption);
    parser.addOption(methodOption);

    //Load data
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));
    QFile t_fileInv(parser.value(invFileOption));

    qint32 nave = parser.value(numberAveragesOption).toInt();
    float snr = parser.value(snrOption).toFloat();
    float lambda2 = pow(1.0f / snr, 2.0f);

    QString method = parser.value(methodOption);

    bool dSPM = false;
    bool sLORETA = false;

    if(method == "dSPM") {
        dSPM = true;
    } else if(method == "sLORETA") {
        sLORETA = true;
    }

    //
    //   Read the data first
    //
    FiffEvokedSet evokedSet(t_fileEvoked);

    //
    //   Then the inverse operator
    //
    MNEInverseOperator inv_raw(t_fileInv);

    //
    //   Iterate over found data sets
    //
    for(qint32 setno = 0; setno < evokedSet.evoked.size(); ++setno)
    {
        printf(">> Computing inverse for %s data set <<\n", evokedSet.evoked[setno].comment.toUtf8().constData());
        //
        //   Set up the inverse according to the parameters
        //
        if (nave < 0)
            nave = evokedSet.evoked[setno].nave;

        MNEInverseOperator inv = inv_raw.prepare_inverse_operator(nave,lambda2,dSPM,sLORETA);
        //
        //   Pick the correct channels from the data
        //
        FiffEvokedSet newEvokedSet = evokedSet.pick_channels(inv.noise_cov->names);

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

        MatrixXd trans = reginv*inv.eigen_fields->data*inv.whitener*inv.proj*evokedSet.evoked[setno].data;
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
        float tmin = ((float)evokedSet.evoked[setno].first) / evokedSet.info.sfreq;
        float tstep = 1/evokedSet.info.sfreq;

        std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << sol.block(0,0,10,10) << std::endl;
        printf("tmin = %f s\n", tmin);
        printf("tstep = %f s\n", tstep);
    }

    return a.exec();
}
