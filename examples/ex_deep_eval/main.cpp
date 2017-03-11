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
#include <deep/deepeval.h>

#include <iostream>


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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CNTK;
using namespace Eigen;
using namespace DEEPLIB;
using namespace QtCharts;


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

    Deep deep_v2;

    if(deep_v2.loadModel("./mne_deep_models/examples/output/models/ex_deep_one_hidden", DeviceDescriptor::CPUDevice())) {
        fprintf(stderr, "\n##### Run evaluation using pre-trained model on CPU. #####\n");

        size_t inDim = deep_v2.inputDimensions();
        int samples = 1;

        fprintf(stderr, "Input Dimension %d\n", (int)inDim);

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
        deep_v2.evalModel(DeviceDescriptor::CPUDevice(), inputs, outputs);

        std::cout << "outputs\n" << outputs << std::endl;


        deep_v2.saveModel("./mne_deep_models/examples/ex_deep_one_hidden.v2");


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


    //
    // Eval v1
    //
    {
        DeepEval deepTest(QApplication::applicationDirPath() + "/mne_deep_models/examples/output/models/ex_deep_one_hidden");

        //
        // Generate dummy input values in the appropriate structure and size
        //
        std::vector<float> inputs, outputs;

        std::cout << "inputs\n";
        for (int i = 0; i < deepTest.inputDimensions(); i++) {
            fprintf(stderr, "%d ", (i % 255));
            inputs.push_back(static_cast<float>(i % 255));
        }
        std::cout << "\n";

        //
        // Evaluate the model
        //
        deepTest.evalModel(inputs, outputs);

        //
        // Output the results
        //
        std::cout << "outputs\n";
        for (auto& value : outputs) {
            fprintf(stderr, "%f ", value);
        }
        std::cout << "\n";
    }
    return a.exec();
}
