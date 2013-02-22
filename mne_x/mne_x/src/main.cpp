//=============================================================================================================
/**
* @file		main.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "FormFiles/mainsplashscreen.h"
#include "FormFiles/mainwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CSART;


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
    QApplication app(argc, argv);

    QPixmap pixmap(":/images/splashscreen.png");
    MainSplashScreen *splashscreen = new MainSplashScreen(pixmap);
    splashscreen->show();

    //ToDo Debug Some waiting stuff to see splash screen -> remove this in final release
    int time = 100;
    for(int i=0; i < time;++i)
    {
        int p = (i*100)/time;
        splashscreen->showMessage("Loading modules.."+ QString::number(p)+"%");
    }

    MainWindow mainWin;
    mainWin.show();

    splashscreen->finish(&mainWin);

    return app.exec();
}
