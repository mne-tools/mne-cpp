//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implements the main() application function.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/imagesc.h>
#include <disp/plot.h>
#include <disp/rtplot.h>

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QImage>
#include <QGraphicsView>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;


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

    //ImageSc Test
    qint32 width = 300;
    qint32 height = 400;
    MatrixXd mat(width,height);

    for(int i = 0; i < width; ++i)
        for(int j = 0; j < height; ++j)
            mat(i,j) = ((double)(i+j))/698.0;//*0.1-1.5;

    ImageSc imagesc(mat);
    imagesc.setTitle("Test Matrix");
    imagesc.setXLabel("X Axes");
    imagesc.setYLabel("Y Axes");

    imagesc.setColorMap("Hot");//imagesc.setColorMap("Jet");//imagesc.setColorMap("RedBlue");//imagesc.setColorMap("Bone");//imagesc.setColorMap("Jet");//imagesc.setColorMap("Hot");

    imagesc.setWindowTitle("Corresponding function to MATLABs imagesc");
    imagesc.show();

    //Plot Test
    qint32 t_iSize = 100;
    VectorXd vec(t_iSize);
    for(int i = 0; i < t_iSize; ++i)
    {
        double t = 0.01 * i;
        vec[i] = sin(2 * 3.1416 * 4 * t); //4 Hz
    }

    Plot plot(vec);

    plot.setTitle("Test Plot");
    plot.setXLabel("X Axes");
    plot.setYLabel("Y Axes");

    plot.setWindowTitle("Corresponding function to MATLABs plot");
    plot.show();


    RtPlot rtplot(vec);

    rtplot.setTitle("Test Plot");
    rtplot.setXLabel("X Axes");
    rtplot.setYLabel("Y Axes");

    rtplot.setWindowTitle("Rt Plot");
    rtplot.show();




    return a.exec();
}
