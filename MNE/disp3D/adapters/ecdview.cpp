//=============================================================================================================
/**
* @file     ecdview.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecdview.h"

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/ecd_set.h>
#include <mne/mne_bem.h>

#include <disp3D/view3D.h>
#include <disp3D/model/data3Dtreemodel.h>
#include <disp3D/control/control3dwidget.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECDView::ECDView(const DipoleFitSettings& dipFitSettings, const ECDSet& ecdSet, QWidget* parent)
: QWidget(parent)
, m_p3DView(View3D::SPtr(new View3D()))
, m_pData3DModel(Data3DTreeModel::SPtr(new Data3DTreeModel()))
{
    //Init 3D View
    m_p3DView->setModel(m_pData3DModel);

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    m_pControl3DView = Control3DWidget::SPtr(new Control3DWidget(this, slControlFlags));

    m_pControl3DView->init(m_pData3DModel, m_p3DView);

    //Read mri transform
    QFile file(dipFitSettings.mriname);
    ECDSet ecdSetTrans = ecdSet;

    if(file.exists()) {
        FiffCoordTrans coordTrans(file);

        std::cout << std::endl << "coordTrans" << coordTrans.trans;
        std::cout << std::endl << "coordTransInv" << coordTrans.invtrans;

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

    //Create widget GUI
    QGridLayout *mainLayoutView = new QGridLayout;
    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_p3DView.data());
    pWidgetContainer->setMinimumSize(400,400);
    mainLayoutView->addWidget(pWidgetContainer,0,0);
    mainLayoutView->addWidget(m_pControl3DView.data(),0,1);

    this->setLayout(mainLayoutView);
}


//*************************************************************************************************************

ECDView::~ECDView()
{
}
