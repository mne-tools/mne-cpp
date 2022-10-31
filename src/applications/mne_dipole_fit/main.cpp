//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/dipole_fit.h>

#include <utils/generics/applicationlogger.h>

#include <mne/mne_bem.h>

#include <disp3D/viewers/ecdview.h>

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace MNELIB;
using namespace UTILSLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_dipole_fit application.
 * By default, main has the storage class extern.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    #ifdef STATICBUILD
    Q_INIT_RESOURCE(disp3d);
    #endif

    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication app(argc, argv);

    DipoleFitSettings settings(&argc,argv);
    DipoleFit dipFit(&settings);
    ECDSet set = dipFit.calculateFit();

    ECDView::SPtr pEcdViewer;
    if(settings.gui) {
        pEcdViewer = ECDView::SPtr(new ECDView(settings, set));
        pEcdViewer->show();
    }

    /*
     * Saving...
     */
    if (!set.save_dipoles_dip(settings.dipname))
        printf("Dipoles could not be safed to %s.",settings.dipname.toUtf8().data());
    if (!set.save_dipoles_bdip(settings.bdipname))
        printf("Dipoles could not be safed to %s.",settings.bdipname.toUtf8().data());

    /*
     * Test - Reading again
     */
    ECDSet::read_dipoles_dip(settings.dipname);

    return app.exec();
}
