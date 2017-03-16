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
#include <deep/deepviewer.h>
#include <deep/deepmodelcreator.h>
#include <deep/deepmodelviewer/deepmodelviewer.h>

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
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DEEPLIB;
using namespace QtCharts;
using namespace CNTK;


//*************************************************************************************************************
//=============================================================================================================
// STATIC FUNCTIONS
//=============================================================================================================

// Helper function to generate a random data sample
void generate_random_data_samples(int sample_size, int feature_dim, int num_classes, MatrixXf& X, MatrixXf& Y)
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
//    Q_INIT_RESOURCE(pathstroke);

    QApplication a(argc, argv);

    bool smallScreen = QApplication::arguments().contains("-small-screen");

    DeepModelViewerWidget deepModelViewer(smallScreen);
    QList<QWidget *> widgets = deepModelViewer.findChildren<QWidget *>();
    foreach (QWidget *w, widgets) {
        w->setAttribute(Qt::WA_AcceptTouchEvents);
    }

    if (smallScreen)
        deepModelViewer.showFullScreen();
    else
        deepModelViewer.show();

#ifdef QT_KEYPAD_NAVIGATION
    QApplication::setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    return a.exec();



//    DeviceDescriptor device = DeviceDescriptor::CPUDevice();


//    //
//    // Example 1
//    //
//    std::cout  << std::endl << "<<< Example 1 >>>" << std::endl << std::endl;

//    size_t input_dim = 2;
//    size_t num_output_classes = 2;

//    Deep deep;

//    int mysamplesize = 32;
//    MatrixXf features, labels, outputs;;

//    generate_random_data_samples(mysamplesize, static_cast<int>(input_dim), static_cast<int>(num_output_classes), features, labels);

//    std::cout << "\nfeatures\n" << features << std::endl;
//    std::cout << "\nlabels\n" << labels << std::endl;

//    FunctionPtr model_1 = DeepModelCreator::FFN_1(input_dim, num_output_classes, device);
//    deep.setModel(model_1);


//    //
//    // Evaluation / Testing beforehand
//    //
//    qDebug() << "\n Evaluation before training \n";

//    // Generate new data
//    int test_minibatch_size = 25;
//    generate_random_data_samples(test_minibatch_size, static_cast<int>(input_dim), static_cast<int>(num_output_classes), features, labels);

//    deep.evalModel(features, outputs, device);

//    for(int i = 0; i < labels.rows(); ++i) {
//        std::cout << "desired " << labels.row(i) << "; output ";
//        for(int j = 0; j < labels.cols(); ++j) {
//            std::cout << (outputs(i,j) <= 0 ? 0 : 1) << " ";
//        }
//        std::cout << "; (real " << outputs.row(i) << ")" << std::endl;
//    }

//    //
//    // Training
//    //
//    qDebug() << "\n Start training \n";

//    // Initialize the parameters for the trainer
//    int minibatch_size = 25;
//    int num_samples = 20000;

//    QVector<double> vecLoss, vecError;
//    generate_random_data_samples(num_samples, static_cast<int>(input_dim), static_cast<int>(num_output_classes), features, labels);
//    deep.trainModel(features, labels, vecLoss, vecError, minibatch_size, device);

//    qDebug() << "\n Finished training \n";

//    //Plot error
//    QChartView *error_chartView = DeepViewer::linePlot(vecError,"Training Error");
//    error_chartView->show();

//    //Plot loss
//    QChartView *loss_chartView = DeepViewer::linePlot(vecLoss,"Loss Error");
//    loss_chartView->show();

//    //
//    // Evaluation / Testing
//    //

//    qDebug() << "\n Evaluation after training \n";

//    // Generate new data

//    test_minibatch_size = 25;
//    generate_random_data_samples(test_minibatch_size, static_cast<int>(input_dim), static_cast<int>(num_output_classes), features, labels);

//    deep.evalModel(features, outputs, device);

//    for(int i = 0; i < labels.rows(); ++i) {
//        std::cout << "desired " << labels.row(i) << "; output ";
//        for(int j = 0; j < labels.cols(); ++j) {
//            std::cout << (outputs(i,j) <= 0 ? 0 : 1) << " ";
//        }
//        std::cout << "; (real " << outputs.row(i) << ")" << std::endl;
//    }


//    return a.exec();
}
