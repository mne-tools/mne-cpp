//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implements the main() application function.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QScatterSeries>
#include <QCategoryAxis>

#include <deep/deep.h>
#include <deep/deepmodelcreator.h>

#include <disp/deepmodelviewer/deepviewerwidget.h>

#include <iostream>
#include <random>


//*************************************************************************************************************
//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace QtCharts;
using namespace CNTK;
using namespace DEEPLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// STATIC FUNCTIONS
//=============================================================================================================

// Helper function to generate a random data sample
void generateRandomDataSamples(int sample_size, int feature_dim, int num_classes, MatrixXf& X, MatrixXf& Y)
{
    MatrixXi t_Y = MatrixXi::Zero(sample_size, 1);
    for(int i = 0; i < t_Y.rows(); ++i) {
        t_Y(i,0) = rand() % num_classes;
    }

    std::default_random_engine generator;
    std::normal_distribution<float> distribution(0.0,1.0);

    X = MatrixXf::Zero(sample_size, feature_dim);
    for(int i = 0; i < X.rows(); ++i) {
        for(int j = 0; j < X.cols(); ++j) {
            float number = distribution(generator);
            X(i,j) = (number + 3) * (t_Y(i) + 1);
        }
    }

    Y = MatrixXf::Zero(sample_size, num_classes);

    for(int i = 0; i < Y.rows(); ++i) {
        Y(i,t_Y(i)) = 1;
    }
}


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
//    Q_INIT_RESOURCE(images.qrc);

    QApplication a(argc, argv);

    // Create a deep model
    DeviceDescriptor device = DeviceDescriptor::CPUDevice();
    size_t input_dim = 4;
    size_t num_output_classes = 3;
    Deep deep;
    FunctionPtr model = DeepModelCreator::FFN_1(input_dim, num_output_classes, device);
    deep.setModel(model);
    deep.print();

    //
    // Training
    //
    qDebug() << "\n Start training \n";

    // Initialize the parameters for the trainer
    int minibatch_size = 25;
    int num_samples = 20000;

    MatrixXf features, labels;

    QVector<double> vecLoss, vecError;
    generateRandomDataSamples(num_samples, static_cast<int>(input_dim), static_cast<int>(num_output_classes), features, labels);
    deep.trainModel(features, labels, vecLoss, vecError, minibatch_size, device);

    //
    // Create the viewer
    //
    DeepViewerWidget window(model);
    window.showMaximized();

    return a.exec();
}
