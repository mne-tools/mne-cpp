//MATCHING PURSUIT
//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief    Example of reading raw data
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
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include "mainwindow.h"
#include <disp/plot.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>
#include <QDateTime>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

MainWindow* mainWindow = NULL;
qint32 ReadFiffFile();

//*************************************************************************************************************
//=============================================================================================================
// MAIN
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

    //set application settings
    QCoreApplication::setOrganizationName("DKnobl MHenfling");
    QApplication::setApplicationName("MatchingPursuit Viewer");


    //qint32 readFiffFile = ReadFiffFile();

    QList<GaborAtom> myAtomList;
    qint32 it = 100;
    qreal epsilon = 0.01;
    adaptiveMP *adaptiveMp = new adaptiveMP();
    qint32 t_iSize = 512;
    MatrixXd signal (t_iSize, 1);
    MatrixXd residuum = signal;

    //Testsignal
    GaborAtom *testSignal1 = new GaborAtom();//t_iSize, 40, 180, 40, 0.8);
    VectorXd t1 = testSignal1->CreateReal(t_iSize, 40, 180, 40, 0.8);//Samples, Scale, translat, modulat(symmetric to size/2, phase
    VectorXd t2 = testSignal1->CreateReal(t_iSize, 80, 52, 50, PI);
    VectorXd t5 = testSignal1->CreateReal(t_iSize, 20, 57, 95.505, PI);
    VectorXd t6 = testSignal1->CreateReal(t_iSize, 10, 40, 120, 0.5);
    VectorXd t7 = testSignal1->CreateReal(t_iSize, 150, 0, 30, PI/2);
    VectorXd t8 = testSignal1->CreateReal(t_iSize, 256, 128, 12, 0);
    VectorXd t9 = testSignal1->CreateReal(t_iSize, 80, 70, 10, 2.7);
    VectorXd t3 = testSignal1->CreateReal(t_iSize, 180, 40, 40, -0.3);
    VectorXd t4 = testSignal1->CreateReal(t_iSize, 210, 200, 46, 1);
    VectorXd t0 = testSignal1->CreateReal(t_iSize, 56, 58, 60, 0.6);
    VectorXd tSig(t_iSize);

    for(qint32 i = 0; i < t_iSize; i++)
    {
        signal(i, 0) =  10 * t1[i] +  10 * t2[i] + 15 * t5[i] + 2 * cos(qreal(i) / 5.0) + 10 * t8[i] + 10 * t7[i]+ 8 * t6[i] + 5 * t4[i] + 20 *t3[i]+ 11 * t0[i] + 20 * t9[i];
        //signal(i, 0) = 100* t8[i];//2 * cos(qreal(i) / 5.0);

        if(i == 149) signal(i, 0) += 25;
        signal(i, 0) += 7 * (sin(qreal(i*i))/ 15.0);
    }
    //find  maximum of signal
    qreal maximum = 0;

    for(qint32 i = 1; i < t_iSize; i++)
        if(abs(maximum) < abs(signal(i,0)))
            maximum = signal(i,0);
    std::cout << "hoechste Amplitude im Signal:    " << maximum << "\n";

    //plot testsignal
    for(qint32 i = 0; i < t_iSize; i++)
        tSig[i] = signal(i, 0);

    Plot *sPlot = new Plot(tSig);
    sPlot->setTitle("Signalplot");
    sPlot->show();
    //tessignal ende

    //run MP Algorithm
    myAtomList = adaptiveMp->MatchingPursuit(signal, it, epsilon);

    //temporary calculating residue and atoms for plotting
    //residuum = signal;

        for(qint32 i = 0; i < myAtomList.length(); i++)
        {
            GaborAtom gaborAtom = myAtomList.at(i);
            residuum = gaborAtom.Residuum;
            VectorXd bestMatch = gaborAtom.CreateReal(gaborAtom.SampleCount, gaborAtom.Scale, gaborAtom.Translation, gaborAtom.Modulation, gaborAtom.Phase);//256, 20, 57, 95.505, PI);//

            //for(qint32 jj = 0; jj < gaborAtom.SampleCount; jj++)
            //    residuum(jj,0) -= gaborAtom.MaxScalarProduct * bestMatch[jj];

            VectorXd plotResiduum = VectorXd::Zero(t_iSize);

            for(qint32 ij = 0; ij < t_iSize; ij++)
            {
                plotResiduum[ij] = gaborAtom.Residuum(ij,0);
            }

            QString title;          // string which will contain the result
            Plot *rPlot = new Plot(plotResiduum);
            title.append(QString("Resid: %1").arg(i));
            rPlot->setTitle(title);
            //rPlot->show();

            //find  maximum of Atom
            maximum = 0;
            for(qint32 ki = 1; ki < t_iSize; ki++)
                if(abs(maximum) < abs(gaborAtom.MaxScalarProduct * bestMatch[ki]))
                    maximum = gaborAtom.MaxScalarProduct * bestMatch[ki];
            std::cout << "hoechste Amplitude im Atom " << i << ":    " << maximum << "\n";

            //find  maximum of Residuum
            maximum = 0;
            for(qint32 mi = 1; mi < t_iSize; mi++)
                if(abs(maximum) < abs(residuum(mi,0)))
                    maximum = residuum(mi,0);
            std::cout << "hoechste Amplitude im Residuum " << i << ":    " << maximum << "\n";
            //delete gaborAtom;

        }

    //plot result of mp algorithm
    VectorXd approximation = VectorXd::Zero(signal.rows());

    for(qint32 i = 0; i < myAtomList.length(); i++)
    {
        GaborAtom gaborAtom = myAtomList.at(i);//new GaborAtom;
        //paintAtom = myAtomList.at(i);
        qint32 var1 = (gaborAtom.SampleCount);
        qreal var2 = (gaborAtom.Scale);
        qint32 var3 = (gaborAtom.Translation);
        qreal var4 = gaborAtom.Modulation;
        qreal var5 = gaborAtom.Phase;
        //qreal var6 = gaborAtom.MaxScalarProduct;
        std::cout << "Parameter die Residuum bauen:\n   "<< " scale:  "  << var2 << " transl: " << var3 <<" modul: " << var4 <<" phase: " << var5 <<"\n";
        //std::cout << atan(1000000000)*180/PI << "\n";
        approximation += gaborAtom.MaxScalarProduct * gaborAtom.CreateReal(var1, var2, var3, var4, var5);

        QString title;          // string which will contain the title

        //plot atoms found
        Plot *atPlot = new Plot(gaborAtom.CreateReal(var1, var2, var3, var4, var5));
        title.append(QString("Atom: %1").arg(i));
        atPlot->setTitle(title);
        //atPlot->show();
    }

    maximum = 0;
    for(qint32 i = 1; i < t_iSize; i++)
        if(abs(maximum) < abs(approximation[i]))
            maximum = approximation[i];
    std::cout << "hoechste Amplitude in Approximation ohne Residuum:    " << maximum << "\n";

    //plot approximation
    Plot *aPlot = new Plot(approximation);
    aPlot->setTitle("Approximation ohne Residuum");
    aPlot->show();

    for(qint32 i = 0; i < t_iSize; i++)
        approximation[i] += residuum(i,0);

    //ploat approxiamtion and residuum
    Plot *arPlot = new Plot(approximation);
    arPlot->setTitle("Approximation mit Residuum");
    arPlot->show();

    //plot residuum
    VectorXd plotResiduum = VectorXd::Zero(t_iSize);
    for(qint32 i = 0; i < t_iSize; i++)
    {
        plotResiduum[i] = residuum(i,0);
    }
    Plot *rPlot = new Plot(plotResiduum);
    rPlot->setTitle("Residuum");
    rPlot->show();

    //mainWindow = new MainWindow();
    //mainWindow->show();

    return a.exec();
}

qint32 ReadFiffFile ()
{
    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");

    float from = 42.956f;
    float to = 320.670f;

    bool in_samples = false;

    bool keep_comp = true;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
       printf("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
           raw.info.projs[k].active = true;

       printf("%d projection items activated\n",raw.info.projs.size());
       //
       //   Create the projector
       //
       fiff_int_t nproj = raw.info.make_projector(raw.proj);

       if (nproj == 0)
           printf("The projection vectors do not apply to these channels\n");
       else
           printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
       printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
       qDebug() << "This part needs to be debugged";
       if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
       {
          raw.info.set_current_comp(dest_comp);
          printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
       }
       else
       {
          printf("Could not make the compensator\n");
          return -1;
       }
    }
    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;
    MatrixXd data;
    MatrixXd times;
    if (in_samples)
        readSuccessful = raw.read_raw_segment(data, times, (qint32)from, (qint32)to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(data, times, from, to, picks);

    if (!readSuccessful)
    {
       printf("Could not read raw segment.\n");
       return -1;
    }

    printf("Read %d samples.\n",(qint32)data.cols());


    std::cout << data.block(0,0,10,10) << std::endl;

    return -1;
}
