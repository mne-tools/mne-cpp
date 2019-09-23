//=============================================================================================================
/**
* @file     main.cpp
* @author   Juan Garcia-Prieto <Juan.GarciaPrieto@uth.tmc.edu> <juangpc@gmail.com>;
*           Wayne Mead <wayne.mead@uth.tmc.edu> <wayne.isk@gmail.com>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           John C. Mosher <John.C.Mosher@uth.tmc.edu> <jcmosher@gmail.com>;
* @version  1.0
* @date     September, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Juan Garcia-Prieto and Matti Hamalainen. All rights reserved.
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
* @brief     Application for anonymizing patient and personal health information from a fiff file.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontroller.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================


/**
 * @brief Creates a QApplication or a QCoreApplication according tu user's preference for a command line or a
 *  GUI application.
 * @details  Handles input arguments and searches for a "--no-gui" option. If found, this will create a
 *  QCoreApplication so that main can execute the appplication as a command line one. If not found, it creates a
 * or a QApplication depending on
 *  QApplication so that main can execute a GUI.
 * @see QT Documentation
 * @see https://doc.qt.io/qt-5/qapplication.html#details
 * @param [in] argc (argument count) number of arguments on the command line.
 * @param [in] argv (argument vector) an array of pointers to arrays of characters.
 * @return Pointer to a QApplication or a QCoreApplication.
 */
QCoreApplication* createApplication(int &argc, char *argv[])
{
    //when gui is ready remove this line
    return new QCoreApplication(argc, argv);

//    for (int i = 1; i < argc; ++i)
//        if (!qstrcmp(argv[i], "--no-gui"))
//            return new QCoreApplication(argc, argv);
//    return new QApplication(argc, argv);
}


/**
* The function main marks the entry point of the program.
* By default, main has the storage class extern.
*
* @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char* argv[])
{
    QScopedPointer<QCoreApplication> qtApp(createApplication(argc, argv));

    if (qobject_cast<QApplication *>(qtApp.data())) {
        // to do -> develop GUI version...
        //create reader object and parse data
        //MainWindow w;
        //w.show();
    } else {
        // start non-GUI version...
        MNEANONYMIZE::SettingsController controller(reinterpret_cast<QCoreApplication *>(&qtApp));
    }

    return qtApp->exec();
}

