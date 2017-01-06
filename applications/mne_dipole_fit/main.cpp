//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implements the mne_dipole_fit application.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/dipole_fit.h>

#include <mne/mne_bem.h>

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>
#include <disp3D/model/items/sourceactivity/ecddatatreeitem.h>
#include <disp3D/model/data3Dtreemodel.h>

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
* The function main marks the entry point of the mne_dipole_fit application.
* By default, main has the storage class extern.
*
* @param [in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DipoleFitSettings settings(&argc,argv);
    DipoleFit dipFit(&settings);
    ECDSet set = dipFit.calculateFit();

    /*
    * Perform Head->MRI coord. transformation
    */
    FiffCoordTrans coordTrans(QFile("./MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif"));

    std::cout << std::endl << "coordTrans" << coordTrans.trans;
    std::cout << std::endl << "coordTransInv" << coordTrans.invtrans;

    for(int i = 0; i < set.size() ; ++i) {
        MatrixX3f dipoles(1, 3);
        //transform location
        dipoles(0,0) = set[i].rd(0);
        dipoles(0,1) = set[i].rd(1);
        dipoles(0,2) = set[i].rd(2);

        dipoles = coordTrans.apply_trans(dipoles);

        set[i].rd(0) = dipoles(0,0);
        set[i].rd(1) = dipoles(0,1);
        set[i].rd(2) = dipoles(0,2);

        //transform orientation
        dipoles(0,0) = set[i].Q(0);
        dipoles(0,1) = set[i].Q(1);
        dipoles(0,2) = set[i].Q(2);

        dipoles = coordTrans.apply_trans(dipoles);

        set[i].Q(0) = dipoles(0,0);
        set[i].Q(1) = dipoles(0,1);
        set[i].Q(2) = dipoles(0,2);
    }

    /*
    * Visualize the dipoles
    */
    //Load FS set
    SurfaceSet tSurfSet ("sample", 2, "orig", "./MNE-sample-data/subjects");
    AnnotationSet tAnnotSet ("sample", 2, "orig", "./MNE-sample-data/subjects");

    //Read and show BEM
    QFile t_fileBem("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_Bem(t_fileBem);

    //Create 3D data model and add data to model
    Data3DTreeModel::SPtr p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());

    ECDSet::SPtr pSet = ECDSet::SPtr(new ECDSet(set));
    p3DDataModel->addBemData("sample", "BEM", t_Bem);
    p3DDataModel->addSurfaceSet("sample", "Dipole test", tSurfSet, tAnnotSet);
    p3DDataModel->addDipoleFitData("sample", "Dipole test", pSet);

    //Create the 3D view
    View3D::SPtr testWindow = View3D::SPtr(new View3D());
    testWindow->setModel(p3DDataModel);
    testWindow->show();

    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->init(p3DDataModel, testWindow);
    control3DWidget->show();

    /*
    * Saving...
    */
    if (!set.save_dipoles_dip(settings.dipname))
        printf("Dipoles could not be safed to %s.",settings.dipname.toLatin1().data());
    if (!set.save_dipoles_bdip(settings.bdipname))
        printf("Dipoles could not be safed to %s.",settings.bdipname.toLatin1().data());

    /*
    * Reading again - Test
    */
    ECDSet::read_dipoles_dip(settings.dipname);

    return app.exec();
}
