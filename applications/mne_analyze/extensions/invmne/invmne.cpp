//=============================================================================================================
/**
* @file     mne.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the MNE class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "invmne.h"
#include "FormFiles/invmnecontrol.h"


#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>

#include <mne/mne.h>
#include <mne/mne_sourceestimate.h>

#include <utils/mnemath.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVMNEEXTENSION;
using namespace ANSHAREDLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvMNE::InvMNE()
: m_pControl(Q_NULLPTR)
, m_pInvMNEControl(Q_NULLPTR)
{

}


//*************************************************************************************************************

InvMNE::~InvMNE()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> InvMNE::clone() const
{
    QSharedPointer<InvMNE> pInvMNEClone(new InvMNE);
    return pInvMNEClone;
}


//*************************************************************************************************************

void InvMNE::init()
{
    m_pInvMNEControl = new InvMNEControl;
}


//*************************************************************************************************************

void InvMNE::unload()
{

}


//*************************************************************************************************************

QString InvMNE::getName() const
{
    return "MNE";
}


//*************************************************************************************************************

QMenu *InvMNE::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *InvMNE::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("MNE"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControl->setMinimumWidth(180);
        m_pControl->setWidget(m_pInvMNEControl);
    }

    return m_pControl;
}


//*************************************************************************************************************

QWidget *InvMNE::getView()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

void InvMNE::calculate()
{
    //*********************************************************************************************************
    // MNE Settings
    //*********************************************************************************************************

    QString sampleEvokedFile("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QString invFile("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif");
    QString method("dSPM");//"MNE" | "dSPM" | "sLORETA"

    float snr = 1.0f;//2.0f
    float lambda2 = pow(1.0f / snr, 2.0f);

    int setno = 0;


    //*********************************************************************************************************
    // LOAD DATA
    //*********************************************************************************************************

    QFile t_fileEvoked(sampleEvokedFile);
    QFile t_fileInv(invFile);

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


    //*********************************************************************************************************
    // Compute MNE
    //*********************************************************************************************************

    printf("Data contains %d sets\n",evokedSet.evoked.size());
    for(qint32 i = 0; i < evokedSet.evoked.size(); ++i) {
        printf("%s\n", evokedSet.evoked[i].comment.toUtf8().constData());
    }

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute MNE for %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", evokedSet.evoked[setno].comment.toUtf8().constData());

    //
    //   Set up the inverse according to the parameters
    //
    qint32 nave = evokedSet.evoked[setno].nave;

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

    qDebug() << "Dimensionalities";
    qDebug() << "sol" << sol.rows() << "x" << sol.cols();
    qDebug() << "evoked" << evokedSet.evoked[setno].data.rows() << "x" << evokedSet.evoked[setno].data.cols();


    printf("tmin = %f s\n", tmin);
    printf("tstep = %f s\n", tstep);

    qDebug() << "TODO: Output needs to be labeled, i.e. mapped to 0 and 1";

//    m_features = evokedSet.evoked[setno].data.transpose();

//    m_labels = sol.transpose();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MNE Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}
