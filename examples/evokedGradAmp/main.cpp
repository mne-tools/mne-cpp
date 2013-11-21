//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

#include <iostream>
#include <vector>
#include <math.h>


#include <fiff/fiff.h>
#include <mne/mne.h>

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;


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
    QCoreApplication a(argc, argv);

    //generate FiffEvokedSet
    QFile t_sampleFile("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    FiffEvokedSet p_FiffEvokedSet(t_sampleFile);

    //mne_ex_evoked_grad_amp.m example
    fiff_int_t coil1,coil2;
    QString one,two;
    QChar lastone,lasttwo;
    qint32 npair = 0;
    MatrixXi pairs;
    fiff_double_t base1,base2;

    //settings
    bool do_baseline = false;
    fiff_int_t b1 = 1;
    fiff_int_t b2 = 1;
    fiff_double_t bmin = 0;
    fiff_double_t bmax = 0.231;

    for(qint16 i=0; i < p_FiffEvokedSet.info.nchan; ++i) {
        //First check the coil types
        coil1 = p_FiffEvokedSet.info.chs.at(i).coil_type;
        coil2 = p_FiffEvokedSet.info.chs.at(i+1).coil_type;
        if (coil1 == coil2 && (coil1 == 2 || coil1 == 3012 || coil1 == 3013)) {
            one = p_FiffEvokedSet.info.ch_names[i];
            two = p_FiffEvokedSet.info.ch_names[i+1];
            lastone = one.at(one.size()-1);
            lasttwo = one.at(two.size()-1);

            //Then the channel names
            if((one.left(3) == "MEG") && (two.left(3) == "MEG") && (one.left(one.size()-1) == two.left(two.size()-1)) && ((one.right(1)=="2" && two.right(1)=="3") || (one.right(1)=="3" && two.right(1)=="2"))) {
                ++npair;
                pairs(npair,0) = i;
                pairs(npair,1) = i+1;
                ++i;
            }
        }
    }

    printf("Computing the amplitudes");
    if(do_baseline) {
        printf("(Baseline = %7.1f ... %7.1f ms)',1000*bmin,1000*bmax)",1000*bmin,1000*bmax);
    }
    printf("...");

    for(qint16 i=0; i < p_FiffEvokedSet.evoked.size()-1; ++i) {
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
        for(qint16 p; p < npair; ++p) {
            ArrayXd tmparray;
            fiff_int_t p0=pairs(p,0),p1=pairs(p,1);

            if(b2 > b1) {
                Matrix<double,1,Dynamic> tmpbase1;
                Matrix<double,1,Dynamic> tmpbase2;

                tmpbase1 = p_FiffEvokedSet.evoked[p].data.block(pairs(p,0),(b2-b1),1,p_FiffEvokedSet.evoked[p].data.cols());
                base1 = tmpbase1.sum()/tmpbase1.rows();
                tmpbase2 = p_FiffEvokedSet.evoked[p].data.block(pairs(p,1),(b2-b1),1,p_FiffEvokedSet.evoked[p].data.cols());
                base2 = tmpbase2.sum()/tmpbase2.rows();

                tmparray = ((p_FiffEvokedSet.evoked[p0].data.row(p0).array()-base1).square()) + ((p_FiffEvokedSet.evoked[p0].data.row(p1).array()-base2).square());
                p_FiffEvokedSet.evoked[p].data.row(p0) = tmparray.matrix();
            }
            else {
                tmparray = ((p_FiffEvokedSet.evoked[p0].data.row(p0).array()).square()) + ((p_FiffEvokedSet.evoked[p0].data.row(p1).array()).square());
                p_FiffEvokedSet.evoked[p].data.row(p0) = tmparray.matrix();
            }
        }
        printf(".");
    }
    printf("[done]\n");

    //Compose the selection name list
    //ToDo...

    //Omit MEG channels but include others
    //ToDo

    //Modify the bad channel list
    //ToDo

    //Do the picking
    //ToDo

    //Optionally write an output file
    //ToDo

    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
