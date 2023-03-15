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
 * @brief     Compute tangential gradient amplitudes from planar gradiometer data
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <Eigen/Core>

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
using namespace Eigen;

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
    parser.setApplicationDescription("Evoked Grad Amp Example");
    parser.addHelpOption();

    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption doBaselineption("doBaseline", "Do baseline correction.", "doBaseline", "false");

    parser.addOption(evokedFileOption);
    parser.addOption(doBaselineption);

    parser.process(a);

    //generate FiffEvokedSet
    QFile t_sampleFile(parser.value(evokedFileOption));
    FiffEvokedSet p_FiffEvokedSet(t_sampleFile);

    //mne_ex_evoked_grad_amp.m example
    fiff_int_t coil1,coil2;
    QString one,two;
    QChar lastone,lasttwo;
    qint32 npair = 0;
    MatrixXi pairs(p_FiffEvokedSet.info.nchan,2);
    fiff_double_t base1,base2;

    QStringList ch_sel_names;

    //settings
    bool do_baseline = false;
    if(parser.value(doBaselineption) == "false" || parser.value(doBaselineption) == "0") {
        do_baseline = false;
    } else if(parser.value(doBaselineption) == "true" || parser.value(doBaselineption) == "1") {
        do_baseline = true;
    }

    fiff_int_t b1 = 1;
    fiff_int_t b2 = 1;
    fiff_double_t bmin = 0;
    fiff_double_t bmax = 0.231;

    for(qint32 i=0; i < p_FiffEvokedSet.info.nchan-1; ++i) {
        //First check the coil types
        coil1 = p_FiffEvokedSet.info.chs.at(i).chpos.coil_type;
        coil2 = p_FiffEvokedSet.info.chs.at(i+1).chpos.coil_type;
        if (coil1 == coil2 && (coil1 == 2 || coil1 == 3012 || coil1 == 3013)) {
            one = p_FiffEvokedSet.info.ch_names[i];
            two = p_FiffEvokedSet.info.ch_names[i+1];
            lastone = one.at(one.size()-1);
            lasttwo = one.at(two.size()-1);

            //Then the channel names
            if((one.left(3) == "MEG") && (two.left(3) == "MEG") && (one.left(one.size()-1) == two.left(two.size()-1)) && ((one.right(1)=="2" && two.right(1)=="3") || (one.right(1)=="3" && two.right(1)=="2"))) {
                pairs(npair,0) = (int) i;
                pairs(npair,1) = i+1;
                ++npair;
                ++i;
            }
        }
    }

    printf("\nComputing the amplitudes");
    if(do_baseline) {
        printf("(Baseline = %7.1f ... %7.1f ms)',1000*bmin,1000*bmax)",1000*bmin,1000*bmax);
    }
    printf("...");

    for(qint32 i=0; i < p_FiffEvokedSet.evoked.size()-1; ++i) {
        if(b2>b1) {
            b1 = (p_FiffEvokedSet.info.sfreq*bmin) - p_FiffEvokedSet.evoked[i].first;
            b2 = (p_FiffEvokedSet.info.sfreq*bmax) - p_FiffEvokedSet.evoked[i].last;
            if(b1 < 1) b1 = 1;
            if(b2 > p_FiffEvokedSet.evoked[i].data.cols()) b2 = p_FiffEvokedSet.evoked[i].data.cols();
        }
        else {
            b1 = 1;
            b2 = 1;
        }

        //go through all pairs
        qint16 p0,p1;
        ArrayXd tmparray;

        for(qint32 p=0; p < npair; ++p) {
            p0 = pairs(p,0);
            p1 = pairs(p,1);

            if(b2 > b1) {
                Matrix<double,1,Dynamic> tmpbase1;
                Matrix<double,1,Dynamic> tmpbase2;

                tmpbase1 = p_FiffEvokedSet.evoked[i].data.block(pairs(p,0),(b2-b1),1,p_FiffEvokedSet.evoked[i].data.cols());
                base1 = tmpbase1.sum()/tmpbase1.rows();
                tmpbase2 = p_FiffEvokedSet.evoked[i].data.block(pairs(p,1),(b2-b1),1,p_FiffEvokedSet.evoked[i].data.cols());
                base2 = tmpbase2.sum()/tmpbase2.rows();

                tmparray = ((p_FiffEvokedSet.evoked[i].data.row(p0).array()-base1).square()) + ((p_FiffEvokedSet.evoked[i].data.row(p1).array()-base2).square()).square();
                p_FiffEvokedSet.evoked[i].data.row(p0) = tmparray.matrix();
            }
            else {
                tmparray = ((p_FiffEvokedSet.evoked[i].data.row(p0).array()).square()) + ((p_FiffEvokedSet.evoked[i].data.row(p1).array()).square());
                p_FiffEvokedSet.evoked[i].data.row(p0) = tmparray.matrix();
            }
        }
        printf(".");
    }
    printf("[done]\n");

    //Compose the selection name list
    for(qint16 i=0; i < npair; ++i) {
        ch_sel_names.append(p_FiffEvokedSet.info.ch_names.at(i));
    }

    //Omit MEG channels but include others
    qint16 k = npair;
    for(qint16 p=0; p < p_FiffEvokedSet.info.nchan; ++p) {
        if((p_FiffEvokedSet.info.channel_type(p) == "grad") || (p_FiffEvokedSet.info.channel_type(p) == "mag")) {
            ++k;
            ch_sel_names.append(p_FiffEvokedSet.info.ch_names.at(p));
        }
    }

    //Modify the bad channel list
    if(!p_FiffEvokedSet.info.bads.isEmpty()) {
        QString one,two;

        for(qint32 i=0; i < npair; ++i) {
            one = p_FiffEvokedSet.info.ch_names.at(pairs(i,0));
            two = p_FiffEvokedSet.info.ch_names.at(pairs(i,1));

            //If one channel of the planar gradiometer is marked bad, add the other to the bad channel list
            if(!p_FiffEvokedSet.info.bads.contains(one) && p_FiffEvokedSet.info.bads.contains(two))
                    p_FiffEvokedSet.info.bads.append(two);
            if(p_FiffEvokedSet.info.bads.contains(one) && !p_FiffEvokedSet.info.bads.contains(two))
                    p_FiffEvokedSet.info.bads.append(one);
        }
    }

    //Do the picking
    p_FiffEvokedSet.pick_channels(ch_sel_names);

    //Optionally write an output file
    //ToDo: implement MNE root function fiff_write_evoked

    return a.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
