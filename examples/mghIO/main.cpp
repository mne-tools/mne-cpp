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

#include <math.h>



// std includes
#include <iostream>
#include <stdint.h>
#include <stdio.h>

// math includes
#include <string>
#include <vector>
#include <array>

//#include <zlib.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QByteArray>
#include <QBitArray>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

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

// prototypes
void printVector(vector<int> vec);

//=============================================================================================================
/**
* read in mgh sample data and store it to mri data structure
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // initialize vars to call loadMGH function
    QString fName = "D:/Repos/mne-cpp/bin/MNE-sample-data/subjects/sample/mri/orig/001.mgh";
//    QDir fDir(fName);
//    QFileInfo ...
//    QString fNameCanonical = fDir.canonicalPath();

    vector<int> slices; // indices of the sclices z to read
    slices.push_back(0);
    int frame = 0; // time frame index
    bool headerOnly = false;

    // call ported freesurfer function to read in file
    cout << "Reading mgh file..." << endl;
    cout << fName.toStdString() << endl;
//    Mri mri =
    QList<Eigen::MatrixXd> listMatSlices = Mgh::loadMGH(fName, slices, frame, headerOnly);


    cout << "Read some slices =" << listMatSlices.size() << endl;




    //ImageSc Test

    MatrixXd mat = listMatSlices[100];
//    qint32 width = 300;
//    qint32 height = 400;


//    for(int i = 0; i < width; ++i)
//        for(int j = 0; j < height; ++j)
//            mat(i,j) = ((double)(i+j))/698.0;//*0.1-1.5;

    ImageSc imagesc(mat);
    imagesc.setTitle("Test Matrix");
    imagesc.setXLabel("X Axes");
    imagesc.setYLabel("Y Axes");

    imagesc.setColorMap("HotNeg2");//imagesc.setColorMap("Jet");//imagesc.setColorMap("RedBlue");//imagesc.setColorMap("Bone");//imagesc.setColorMap("Jet");//imagesc.setColorMap("Hot");

    imagesc.setWindowTitle("Corresponding function to MATLABs imagesc");
    imagesc.show();



    return a.exec();
}

//*************************************************************************************************************

// print vector content to console
void printVector(vector<int> vec)
{
  for (unsigned int i=0; i<vec.size(); ++i)
    cout << vec[i] << ' ';
  cout << '\n';
}
