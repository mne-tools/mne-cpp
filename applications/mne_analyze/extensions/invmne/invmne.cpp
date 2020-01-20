//=============================================================================================================
/**
 * @file     invmne.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the MNE class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "invmne.h"
#include "FormFiles/invmnecontrol.h"

#include <anShared/Management/analyzedata.h>


#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>

#include <mne/mne.h>
#include <mne/mne_sourceestimate.h>

#include <inverse/minimumNorm/minimumnorm.h>

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
using namespace INVERSELIB;


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
    if(!m_pInvMNEControl) {
        m_pInvMNEControl = new InvMNEControl;
        connect(m_pInvMNEControl, &InvMNEControl::calculate_signal, this, &InvMNE::calculate);
    }
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
//    printf("Data contains %d sets\n",evokedSet.evoked.size());
//    for(qint32 i = 0; i < evokedSet.evoked.size(); ++i) {
//        printf("%s\n", evokedSet.evoked[i].comment.toUtf8().constData());
//    }

    //*********************************************************************************************************
    // MNE Settings
    //*********************************************************************************************************

    QString evokedFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QString invFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif");

    float snr = 1.0f;//2.0f
    QString method("dSPM");//"MNE" | "dSPM" | "sLORETA"

    float lambda2 = pow(1.0f / snr, 2.0f);

    int setno = 0;


    //*********************************************************************************************************
    // LOAD DATA
    //*********************************************************************************************************

    QFile t_fileEvoked(evokedFile);
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
    QPair<QVariant, QVariant> baseline(QVariant(), 0);

    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return;

    //
    //   Read the inverse operator
    //
    MNEInverseOperator inverse_operator(t_fileInv);


    //*********************************************************************************************************
    // Compute MNE
    //*********************************************************************************************************


    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute MNE for %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", evoked.comment.toUtf8().constData());

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MNE Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Store MNE Solution
    //*********************************************************************************************************

    std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
    printf("tmin = %f s\n", sourceEstimate.tmin);
    printf("tstep = %f s\n", sourceEstimate.tstep);

    globalData()->addSTC(sourceEstimate);
}
