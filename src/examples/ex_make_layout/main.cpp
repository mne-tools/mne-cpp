//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Builds example for making a 2D layout from 3D points
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <vector>
#include <math.h>

#include <utils/layoutmaker.h>
#include <utils/layoutloader.h>
#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
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
    //
    // Please notice that this example only works in release mode.
    // Debug mode somehow corrupts the simplex coder. ToDo: Fix this!
    //
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Make Layout Example");
    parser.addHelpOption();
    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption chKindOption("coilType", "The coil type <out>.", "coilType", "3012"); // 3012 = FIFFV_COIL_VV_PLANAR_T1, use coil type instead of kind because this way we can distinguish between different layers (outer, inner, etc.), see fiff_constants for details FIFFV_REF_MEG_CH FIFFV_COIL_BABY_REF_MAG FIFFV_COIL_BABY_MAG
    QCommandLineOption outputOption("fileOut", "The output file <out>.", "out", "makeLayout_default.lout");
    QCommandLineOption mirrorxOption("mirrorX", "Mirror final layout along x-axis <mirrorX>.", "mirrorX", "0");
    QCommandLineOption mirroryOption("mirrorY", "Mirror final layout along <-axis <mirrorY>.", "mirrorY", "0");

    parser.addOption(inputOption);
    parser.addOption(outputOption);
    parser.addOption(chKindOption);
    parser.addOption(mirrorxOption);
    parser.addOption(mirroryOption);
    parser.process(a);

    //Read 3D locations
    if(!parser.value(inputOption).contains(".fif") && !parser.value(inputOption).contains(".elc")) {
        qDebug()<<"MakeLayout::Main - Input file type not supported";
        return 0;
    }

    QList<QVector<float> > inputPoints;
    QList<QVector<float> > outputPoints;
    QList<QVector<float> > channel2DData;
    QStringList names;

    if(parser.value(inputOption).contains(".fif")) {
        QFile t_fileRaw(parser.value(inputOption));

        FiffRawData raw(t_fileRaw);
        FiffInfo fiffInfo = raw.info;

        int chKind = parser.value(chKindOption).toInt();

        for(int i = 0; i<fiffInfo.ch_names.size(); i++) {
            int type = fiffInfo.chs.at(i).chpos.coil_type;

            if(type == chKind) {
                QVector<float> temp;
                float x = fiffInfo.chs.at(i).chpos.r0[0] * 100.0f;
                float y = fiffInfo.chs.at(i).chpos.r0[1] * 100.0f;
                float z = fiffInfo.chs.at(i).chpos.r0[2] * 100.0f;

                temp.append(x);
                temp.append(y);
                temp.append(-z);
                inputPoints.append(temp);

                qDebug() << x << " " << y << " " << z;

                names << fiffInfo.ch_names.at(i);
            }
        }
    }

    if(parser.value(inputOption).contains(".elc")) {
        QString unit;

        LayoutLoader::readAsaElcFile(parser.value(inputOption), names, inputPoints, channel2DData, unit);
    }

    qDebug() << "Read the following 3D coordinates from file:";
    qDebug() << names;
    qDebug() << inputPoints;

    // convert 3D points to layout and write to file
    if(inputPoints.size() > 0) {
        float prad = 60.0;
        float width = 5.0;
        float height = 4.0;

        QFile out(parser.value(outputOption));

        int numberTries = 0;
        while(numberTries < 10) {
            if(!LayoutMaker::makeLayout(inputPoints,
                                       outputPoints,
                                       names,
                                       out,
                                       true,
                                       prad,
                                       width,
                                       height,
                                       true,
                                       (bool)parser.value(mirrorxOption).toInt(),
                                       (bool)parser.value(mirrorxOption).toInt())) {
                numberTries++;
            } else {
                numberTries = 11;
            }
        }
    }

    return a.exec();
}
