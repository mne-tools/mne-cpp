//=============================================================================================================

/**
* @file     histogram.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen and Matti Hamalainen. All rights reserved.
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
* @brief    Example of reading raw data
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <math.h>
#include <ctime>
#include <cstdlib>
#include <Eigen/Dense>
#include <string>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mnemath.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace std;
QT_CHARTS_USE_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================

//sineWaveGenerator used for debugging purposes only
#ifndef M_PI
const double M_PI = 3.14159265358979323846;
#endif

void findDisplayAndExponent (QVector<double> resultClassLimits, QVector<double> resultDisplayValues, QVector<int> resultExponentValues, int classAmount)
{
    resultDisplayValues.clear();
    resultDisplayValues.resize(classAmount+1);
    resultExponentValues.clear();
    resultExponentValues.resize(classAmount+1);
    double classLimit{0.0},
           limitDisplayValue{0.0},
           doubleExponentValue{0.0};
    int    limitExponentValue{0};
    for (int ir=0; ir <= classAmount; ir++)
    {
        classLimit = resultClassLimits.at(ir);
        qDebug() << "classLimit = " <<classLimit;
        if (classLimit == 0.0)      //mechanism to guard against evaluation of log(0.0) which is negative infinity
        {
            doubleExponentValue = 0.0;
        }
        else
        {
            doubleExponentValue = log10(abs(classLimit));                             //return the exponent value in double
            qDebug()<< "doubleExponentValue" << doubleExponentValue;
        }
        limitExponentValue = round(doubleExponentValue);                   //round the exponent value to the nearest signed integer
        qDebug()<< "limitExponentValue" << limitExponentValue;
        limitDisplayValue = classLimit * (pow(10,-(limitExponentValue)));    //display value is derived from multiplying class limit with inverse 10 to the power of negative exponent
        qDebug()<< "limitDisplayValue" << limitDisplayValue;
        resultDisplayValues[ir] = limitDisplayValue;                  //append the display value to the return vector
        resultExponentValues[ir] = limitExponentValue;                //append the exponent value to the return vector
        qDebug() << "Display Value = " << resultDisplayValues[ir];
        qDebug() << "Exponent Value = " << resultExponentValues[ir];
    }
    qDebug() << "Raw Display Value Vector" << resultDisplayValues;
    qDebug() << "Raw Exponent Value Vector" << resultExponentValues;

    int lowestExponentValue{0};
    for (int ir=0; ir <= classAmount; ir++)
    {
        if (resultExponentValues[ir] < lowestExponentValue)
        {
            lowestExponentValue = resultExponentValues.at(ir);  //find lowest exponent value to normalize the other display values
        }
    }
    qDebug() << "lowestExponentValue =" << lowestExponentValue;
    for (int ir=0; ir <= classAmount; ir++)
    {
        while (resultExponentValues[ir] > lowestExponentValue)     //normalize the values by multiplying the display value by 10 and reducing the exponentValue by 1 until exponentValue reach the lowestExponentValue
        {
            resultDisplayValues[ir] = resultDisplayValues[ir] * 10;
            resultExponentValues[ir]--;
        }
    }
    qDebug() << "Final Display Value Vector" << resultDisplayValues;
    qDebug() << "Final Exponent Value Vector" << resultExponentValues;
}

Eigen::VectorXd sineWaveGenerator(double amplitude, double xStep, int xNow, int xEnd)
{
    int iterateAmount = (xEnd-xNow)/xStep;
    Eigen::VectorXd sineWaveResultOriginal(iterateAmount);
    Eigen::VectorXd sineWaveResult = sineWaveResultOriginal.transpose();
    double sineResult;
    double omega = 2.0*M_PI;
    int iterateCount = 0;
    for (double step = xNow; step < xEnd; step +=xStep)
    {
        sineResult = amplitude* (sin(omega * step));
        sineWaveResult(iterateCount) = sineResult;
        iterateCount++;
    }
    return sineWaveResult;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");

    float from = 40.0f;
    float to = 46.01f;

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
    Eigen::VectorXd dataSine;
    dataSine = sineWaveGenerator(1.0e-12,(1.0/1000), 0.0, 1.0);

    // histogram calculation
    bool bMakeSymmetrical;
    bMakeSymmetrical = false;      //bMakeSymmetrical option: false means data is unchanged, true means histogram x axis is symmetrical to the right and left
    int classAmount = 10;          //initialize the amount of classes and class frequencies
    double inputGlobalMin = 0.0,
           inputGlobalMax = 0.0;
    QVector<double> resultClassLimits;
    QVector<int> resultFrequency;
    MNEMath::histcounts(dataSine,bMakeSymmetrical, classAmount, resultClassLimits, resultFrequency, inputGlobalMin, inputGlobalMax );   //user input to normalize and sort the data matrix

    //below is the function for printing the results on command prompt (for debugging purposes)
    double lowerClassLimit,
           upperClassLimit;
    char format = 'g';              //format for the histogram for better readability
    int    classFreq,
           totalFreq{0},
           precision = 1;           //format for the amount digits shown in the histogram

    QVector<double> resultDisplayValues;
    QVector<int> resultExponentValues;
    qDebug() << "resultClassLimits = " <<resultClassLimits;
    findDisplayAndExponent (resultClassLimits, resultDisplayValues, resultExponentValues, classAmount);

    //  Start of Qtchart histogram display
    QBarSet *set = new QBarSet("Class");
    QStringList categories;
    QString currentLimits;
        qDebug() << "Lower Class Limit\t Upper Class Limit \t Frequency ";
    for (int kr=0; kr < resultClassLimits.size()-1; kr++)
    {
        lowerClassLimit = resultClassLimits.at(kr);
        upperClassLimit = resultClassLimits.at(kr+1);
        classFreq = resultFrequency.at(kr);
        qDebug() << lowerClassLimit << " \t\t " << upperClassLimit << "\t\t" << classFreq;
        currentLimits = ((QString::number(resultDisplayValues[kr], format, precision) + " to " + (QString::number(resultDisplayValues[kr+1], format, precision))));
        qDebug() << "currentLimits" << currentLimits;
        categories << currentLimits;
        totalFreq = totalFreq + classFreq;
        *set << classFreq;
    }
    qDebug() << "Total Frequency = " << totalFreq;

    //  Start of Qtchart histogram display
    QBarSeries *series = new QBarSeries();
    series->append(set);
    qDebug() << "Finished appending series";

    QChart *chart = new QChart();
    qDebug() <<"Finished creating chart";
    chart->addSeries(series);
    chart->setTitle("MNE-CPP Histogram Example");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->createDefaultAxes();
    chart->setAxisX(axis, series);

    chart->legend()->setVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QMainWindow window;
    window.setCentralWidget(chartView);
    window.resize(420, 300);
    window.show();

    std::cout << data.block(0,0,10,10);
    return a.exec();
}



//*************************************************************************************************************
