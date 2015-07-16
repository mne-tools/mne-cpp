//=============================================================================================================
/**
* @file     main.cpp
* @author   Carsten Boensel <carsten.boensel@tu-ilmenau>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Carsten Boensel. All rights reserved.
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
* @brief    Example application for the fs/mgh and /mri library
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_global.h>

#include <fs/blendian.h>
#include <fs/mri.h>
#include <fs/mgh.h>

#include <disp/imagesc.h>
#include <disp/plot.h>
#include <disp/rtplot.h>

//#include <math.h>

// std includes
#include <iostream>
#include <stdint.h>
#include <stdio.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QString>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace std;
using namespace FSLIB;
using namespace Eigen;
using namespace DISPLIB;

//*************************************************************************************************************

#define VERBOSE true

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
* read in mgh sample data, store it to mri data structure, and plot it
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // initialize vars to call loadMGH function
    QString fName = "D:/Repos/mne-cpp/bin/MNE-sample-data/subjects/sample/mri/orig/001.mgh";

    VectorXi slices(3); // indices of the sclices (z dimension) to read
    slices << 99, 100, 101;

    int frame = 0; // time frame index, negativ values are vectors
    bool headerOnly = false;

    // call ported freesurfer function to read in file
    qDebug() << "Reading mgh file...";
    qDebug() << fName;

    Mri mri = Mgh::loadMGH(fName, slices, frame, headerOnly);
//    QList<Eigen::MatrixXd> listMatSlices = Mgh::loadMGH(fName, slices, frame, headerOnly);

    // check dimensions of slices stack
    //    mri.slices.size() == mri.height
    //    mri.slices[0].rows() == mri.width
    //    mri.slices[0].columns() ==  mri.depth

    //ImageSc Demo Plot
    qDebug() << "\nRead" << mri.slices.size() << "slices.\n";
    quint16 sliceIdx = 150;
    MatrixXd mat = mri.slices[sliceIdx]; // chosen slice index
//    qDebug() << "\nRead" << listMatSlices.size() << "slices.\n";
//    quint16 sliceIdx = 150;
//    MatrixXd mat = listMatSlices[sliceIdx]; // chosen slice index

    ImageSc imagesc(mat);
    imagesc.setTitle("Visualization of chosen slice");
    imagesc.setXLabel("X Axes");
    imagesc.setYLabel("Y Axes");

    QList<QString> colorMaps;
    colorMaps << "HotNeg2"  // 0
              << "Jet"      // 1
              << "RedBlue"  // 2
              << "Bone"     // 3
              << "Jet"      // 4
              << "Hot";     // 5
    imagesc.setColorMap(colorMaps[3]);

    imagesc.setWindowTitle("Slice Plot");
    imagesc.show();

    return a.exec();
}
