//=============================================================================================================
/**
 * @file     ecdview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    ECDView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecdview.h"

#include "../engine/model/data3Dtreemodel.h"

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/ecd_set.h>
#include <mne/mne_bem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECDView::ECDView(const DipoleFitSettings& dipFitSettings,
                 const ECDSet& ecdSet,
                 QWidget* parent,
                 Qt::WindowFlags f)
: AbstractView(parent, f)
{
    //Read mri transform
    QFile file(dipFitSettings.mriname);
    ECDSet ecdSetTrans = ecdSet;

    if(file.exists()) {
        FiffCoordTrans coordTrans(file);

        for(int i = 0; i < ecdSet.size() ; ++i) {
            MatrixX3f dipoles(1, 3);
            //transform location
            dipoles(0,0) = ecdSet[i].rd(0);
            dipoles(0,1) = ecdSet[i].rd(1);
            dipoles(0,2) = ecdSet[i].rd(2);

            dipoles = coordTrans.apply_trans(dipoles);

            ecdSetTrans[i].rd(0) = dipoles(0,0);
            ecdSetTrans[i].rd(1) = dipoles(0,1);
            ecdSetTrans[i].rd(2) = dipoles(0,2);

            //transform orientation
            dipoles(0,0) = ecdSet[i].Q(0);
            dipoles(0,1) = ecdSet[i].Q(1);
            dipoles(0,2) = ecdSet[i].Q(2);

            dipoles = coordTrans.apply_trans(dipoles, false);

            ecdSetTrans[i].Q(0) = dipoles(0,0);
            ecdSetTrans[i].Q(1) = dipoles(0,1);
            ecdSetTrans[i].Q(2) = dipoles(0,2);
        }
    } else {
        qCritical("ECDView::ECDView - Cannot open FiffCoordTrans file");
    }

    //Add ECD data
    m_pData3DModel->addDipoleFitData("sample", QString("Set %1").arg(dipFitSettings.setno), ecdSetTrans);

    //Read and show BEM
    QFile t_fileBem(dipFitSettings.bemname);

    if(t_fileBem.exists()) {
        MNEBem t_Bem(t_fileBem);
        m_pData3DModel->addBemData("sample", "BEM", t_Bem);
    } else {
        qCritical("ECDView::ECDView - Cannot open MNEBem file");
    }
}

//=============================================================================================================

ECDView::~ECDView()
{
}
