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
* @brief    Example of an FreeSurfer Surface application
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>

#include <fs/surfaceset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace DISP3DLIB;


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

    QStringList args = QCoreApplication::arguments();
    int w_pos = args.indexOf("-width");
    int h_pos = args.indexOf("-height");
    int w, h;
    if (w_pos >= 0 && h_pos >= 0)
    {
        bool ok = true;
        w = args.at(w_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse width argument:" << args;
            return 1;
        }
        h = args.at(h_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse height argument:" << args;
            return 1;
        }
    }

    //
    // pial
    //
    SurfaceSet tSurfSetPial ("sample", 2, "pial", "./MNE-sample-data/subjects");

    View3D::SPtr t_BrainView = View3D::SPtr(new View3D());
    t_BrainView->addSurfaceSet("Subject01", "pial", tSurfSetPial);

    //
    // inflated
    //
    SurfaceSet tSurfSetInflated ("sample", 2, "inflated", "./MNE-sample-data/subjects");  
    t_BrainView->addSurfaceSet("Subject01", "inflated", tSurfSetInflated);

    //
    // orig
    //
    SurfaceSet tSurfSetOrig ("sample", 2, "orig", "./MNE-sample-data/subjects");
    t_BrainView->addSurfaceSet("Subject01", "orig", tSurfSetOrig);

    //
    // white
    //
    SurfaceSet tSurfSetWhite ("sample", 2, "white", "./MNE-sample-data/subjects");
    t_BrainView->addSurfaceSet("Subject01", "white", tSurfSetWhite);

    t_BrainView->show();

    //3D control
    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    control3DWidget->setView3D(t_BrainView);
    control3DWidget->show();

    return a.exec();
}
