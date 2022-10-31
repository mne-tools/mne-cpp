//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Example for read an evoked from fiff file.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Evoked Example");
    parser.addHelpOption();

    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption evokedIdxOption("aveIdx", "The average <index> to choose from the average file.", "index", "2");
    QCommandLineOption useCTFCompOption("useCTFComp", "Use the CTF compensator, if available.");

    parser.addOption(evokedFileOption);
    parser.addOption(evokedIdxOption);

    parser.process(a);

    //generate FiffEvoked object
    QFile t_sampleFile(parser.value(evokedFileOption));
    FiffEvoked p_FiffEvoked(t_sampleFile,QVariant(parser.value(evokedIdxOption)));

    //Select the head coordinate system
    bool use_ctf_head = parser.isSet(useCTFCompOption);
    FiffCoordTrans meg_trans;

    if(use_ctf_head) {
        if(p_FiffEvoked.info.ctf_head_t.isEmpty())
           std::cout << "\nNo CTF head transformation available" << std::endl;
        else {
            meg_trans = p_FiffEvoked.info.dev_ctf_t;
            FiffCoordTrans eeg_trans(meg_trans);
            eeg_trans.invert_transform();
            std::cout << "Employing the CTF/4D head coordinate system\n" << std::endl;
        }
    }
    else {
        meg_trans = p_FiffEvoked.info.dev_head_t;
        FiffCoordTrans eeg_trans;
        std::cout << "Employing the Neuromag head coordinate system\n" << std::endl;
    }

    //Transform coil and electrode locations to the desired coordinate frame
    //ToDo: MATLAB root fct fiff_transform_meg_chs and fiff_transform_eeg_chs needs to be implemented

    //Create the coil definitions
    //ToDo: MATLAB root fct mne_add_coil_defs needs to be implemented

    //N.B. If a nonstandard (in MNE sense) coil def file is used, do
    //ToDo: MATLAB root fct mne_load_coil_def, mne_add_coil_defs needs to be implemented

    return a.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
