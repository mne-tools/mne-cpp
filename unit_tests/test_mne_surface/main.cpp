//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    mne Surface test
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>
#include <mne/mne.h>

#include <mne/mne_epoch_data_list.h>

#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

#include <disp3D/brainview.h>

#include <utils/mnemath.h>

#include <iostream>

#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGuiApplication>
#include <QSet>
#include <QElapsedTimer>

//#define BENCHMARK


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
    QGuiApplication a(argc, argv);

//    Vector3f v1, v2;
//    v1 << 1, 2, 5;
//    v2 << 3, 2, 4;
//    Vector3f v3 = v1.cwiseProduct(v2);
//    std::cout << "v3: " << v3 << std::endl;


//    SurfaceSet t_surfSet("./MNE-sample-data/subjects/sample/surf/lh.white", "./MNE-sample-data/subjects/sample/surf/rh.white");

//    VectorXf curv = Surface::read_curv("D:/Data/subjects/mind006/surf/lh.curv");

//    std::cout << "curv\n" << curv.block(curv.size()-10,0,10,1) << std::endl;

//    Surface t_surf("D:/Data/subjects/mind006/surf/lh.pial");

//    std::cout << "Surface nn\n" << t_surf.nn().block(0,0,10,3) << std::endl;

//    qDebug() << "[2]";
//    SurfaceSet t_surfSet2("./MNE-sample-data/subjects/sample/surf", 2, "inflated");

//    qDebug() << "[3]";
//    SurfaceSet t_surfSet3("sample", 2, "inflated", "./MNE-sample-data/subjects");

//    qDebug() << "[4]";
//    AnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");

//    qDebug() << "[5]";



//    BrainView t_brainView("sample", 2, "pial", "./MNE-sample-data/subjects");
    BrainView t_brainView("sample", 2, "pial", "aparc.a2009s", "./MNE-sample-data/subjects");

    if (t_brainView.stereoType() != QGLView::RedCyanAnaglyph)
        t_brainView.camera()->setEyeSeparation(0.3f);
    QStringList args = QCoreApplication::arguments();
    int w_pos = args.indexOf("-width");
    int h_pos = args.indexOf("-height");
    if (w_pos >= 0 && h_pos >= 0)
    {
        bool ok = true;
        int w = args.at(w_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse width argument:" << args;
            return 1;
        }
        int h = args.at(h_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse height argument:" << args;
            return 1;
        }
        t_brainView.resize(w, h);
    }
    else
    {
        t_brainView.resize(800, 600);
    }
    t_brainView.show();

    return a.exec();
}
