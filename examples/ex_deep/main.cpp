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
#include <QScatterSeries>
#include <QCategoryAxis>

#include <deep/deep.h>
#include <deep/deepmodelcreator.h>

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
    QApplication a(argc, argv);

//    Deep deepTest;
//    deepTest.testClone();
//    deepTest.exampleTrain();

    QString fileName("./mne_deep_models/trainModel.v2");
    DeviceDescriptor device = DeviceDescriptor::CPUDevice();

    const size_t inputDim = 2;
    const size_t numOutputClasses = 1;

    Deep deep;

    // if(!deep.loadModel(fileName, device)) {
        fprintf(stderr, "Constructing model.\n");
        deep.setModel(DeepModelCreator::DNN_1(inputDim, numOutputClasses, device));
    // }



    qDebug() << "InDim" << static_cast<int>(deep.inputDimensions());
    qDebug() << "OutDim" << static_cast<int>(deep.outputDimensions());


    int iterations = 20;
    int batchSize = 50;

    // Train a circle
    float radius = 2.5;
    float current_rad;
    float x_m = 4;
    float y_m = 3;
    float x,y;

    double loss, error;

    Eigen::MatrixXf trainFeatures(batchSize,deep.inputDimensions());
    Eigen::MatrixXf trainTarget(batchSize,deep.outputDimensions());


    std::random_device rd;  // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 6); // define the range

    for(int it = 0; it < iterations; ++it) {
        //
        // Data Generation
        //
        for(int i = 0; i < batchSize; ++i) {

            x = distr(eng); y = distr(eng);
            current_rad = sqrt(pow(x - x_m,2) + pow(y - y_m,2));

            trainFeatures(i,0) = x;
            trainFeatures(i,1) = y;
            trainTarget(i,0) = static_cast<float>(current_rad < radius);
        }

        //
        // Training Generation
        //
        deep.trainMinibatch(trainFeatures, trainTarget, loss, error, device);

        qDebug() << "Iteration:" << it+1 << "; loss" << loss << "; error" << error;
    }


    //
    // Evaluation
    //

    int eval_size = 10;

    Eigen::MatrixXf inputs(eval_size,deep.inputDimensions());
    Eigen::MatrixXf desired(eval_size,deep.outputDimensions());

    for(int i = 0; i < eval_size; ++i) {
        //
        // Data Generation
        //
        x = distr(eng); y = distr(eng);
        current_rad = sqrt(pow(x - x_m,2) + pow(y - y_m,2));

        inputs(i,0) = x;
        inputs(i,1) = y;
        desired(i,0) = static_cast<float>(current_rad < radius);
    }

    std::cout << "inputs\n" << inputs << std::endl;

    MatrixXf outputs;
    deep.evalModel(inputs, outputs, device);

    std::cout << "desired\n" << desired << std::endl;

    std::cout << "outputs\n" << outputs << std::endl;


    deep.saveModel(fileName);


/*
    if(deep.loadModel("./mne_deep_models/examples/output/models/ex_deep_one_hidden", device)) {
        fprintf(stderr, "\n##### Run evaluation using pre-trained model on CPU. #####\n");

        size_t inDim = deep.inputDimensions();
        int samples = 1;
        fprintf(stderr, "Input Dimension %d\n", (int)inDim);

        //
        // Train
        //




//        deep.trainModel();




        //
        // Evaluation
        //

        MatrixXf inputs(samples,inDim);

        int count = 0;
        for (int i = 0; i < samples; i++) {
            for (int j = 0; j < inDim; j++) {
                inputs(i,j) = static_cast<float>(count % 255);
                ++count;
            }
        }

        std::cout << "inputs\n" << inputs << std::endl;

        MatrixXf outputs;
        deep.evalModel(inputs, outputs, device);

        std::cout << "outputs\n" << outputs << std::endl;



        //
        // Visualize
        //

        // Input
        QImage inputImage(28,28,QImage::Format_RGB32);
        for (int i = 0; i < inDim; i++) {
            int gray = inputs(0,i);
            inputImage.setPixelColor(i/28, i%28, QColor(gray,gray,gray));
        }
        QLabel inputView; inputView.setPixmap(QPixmap::fromImage(inputImage));
        inputView.show();

        // Output
        QScatterSeries *series = new QScatterSeries();
        QCategoryAxis *axisX = new QCategoryAxis();
        series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        for (int i = 0; i < outputs.size(); i++) {
            axisX->append(QString("%1").arg(i), i);
            series->append(i,outputs(0,i));
        }
        axisX->setRange(0, 9);
        axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

        QChart *chart = new QChart();
        chart->legend()->hide();
        chart->addSeries(series);
        chart->createDefaultAxes();
        chart->setTitle("Likelyhoods for Number [0-9]");
        chart->setAxisX(axisX);

        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->show();
    }
//*/

    return a.exec();
}
