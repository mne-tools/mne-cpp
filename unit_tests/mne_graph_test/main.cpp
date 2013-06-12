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
* @brief    Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "matrixview.h"



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QImage>
#include <QGraphicsView>


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

//    qint32 x = 255;
//    qint32 y = 255;
//    QImage image(x, y, QImage::Format_RGB32);
//    QRgb value;

//    value = qRgb(189, 149, 39); // 0xffbd9527
//    for(qint32 i = 0; i < x; ++i)
//    {
//        for(qint32 j = 0; j < y; ++j)
//        {
//            value = qRgb(i % 255, j % 255, 100);
//            image.setPixel(i, j, value);
//        }
//    }

////    value = qRgb(122, 163, 39); // 0xff7aa327
////    image.setPixel(0, 1, value);
////    image.setPixel(1, 0, value);

////    value = qRgb(237, 187, 51); // 0xffedba31
////    image.setPixel(2, 1, value);


//    QPixmap pixmap = QPixmap::fromImage(image);

//    QGraphicsScene scene;

////    scene.addText("Hello, world!");
//    scene.addPixmap(pixmap);

//    QGraphicsView view(&scene);
//    view.show();


    MatrixXd test(200,300);

    int count = 200;

    for(int i = 0; i < 200; ++i)
    {
        for(int j = 0; j < 300; ++j)
        {
            test(i,j) = i+j;
            ++count;
        }
    }


    MatrixView mview;

    mview.updateMatrix(test);
    mview.show();


    
    return a.exec();
}
