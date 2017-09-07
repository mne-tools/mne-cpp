//=============================================================================================================
/**
* @file     main.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    Example of using compute shaders for interpolation
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include <mne/mne_sourceestimate.h>
#include <mne/mne_bem.h>
#include <disp3D/engine/model/3dhelpers/custommesh.h>
#include <computeShader/computematerial.h>
#include <computeShader/computeframegraph.h>
#include <computeShader/computeinterpolationcontroller.h>

#include <iostream>

#include <geometryInfo/geometryinfo.h>
#include <interpolation/interpolation.h>
#include <fiff/fiff_constants.h>

#include <cmath>
#include <cstdlib>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//@TODO check for unnecessary includes
#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDateTime>
#include <QFile>

#include <QByteArray>
#include <QRectF>
#include <Qt3DCore>
#include <Qt3DRender>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QSphereGeometry>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace GEOMETRYINFO;
using namespace INTERPOLATION;
using namespace CSH;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DECLARED FUNCTIONS
//=============================================================================================================

QByteArray createVertexBufferFromBemSurface(const MNEBemSurface &tBemSurface);
QByteArray createColorBuffer(const int iVertNum);
MatrixX3f createVertColor(const MatrixXf& vertices, const QColor& color);
QByteArray createWeightMatBuffer(QSharedPointer<SparseMatrix<double> > tInterpolationMatrix);


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

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("ex_interpolation");
    parser.addHelpOption();

    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(evokedIndexOption);

    parser.process(a);

    //acquire sensor positions
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));


    // Load data
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, parser.value(evokedIndexOption).toInt(), baseline);
    if(evoked.isEmpty())
    {
        return 1;
    }


    //acquire surface data
    //QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

    //########################################################################################
    //
    // QT3D INIT
    //
    //########################################################################################

    Qt3DExtras::Qt3DWindow view;

    ComputeInterpolationController *CompController = new ComputeInterpolationController;

    CompController->setInterpolationData(t_sensorSurfaceVV[0],evoked, Interpolation::linear, FIFFV_EEG_CH, 0.03 );
    Eigen::MatrixXf tempMat = evoked.data.cast<float>();
    std::cout << tempMat.rows() << " " << tempMat.cols() <<std::endl;
    CompController->addSignalData(tempMat);

    Qt3DCore::QEntity *rootEntiy = CompController->getRootEntity();
    ComputeFramegraph *pFramegragh = CompController->getComputeFramegraph();




    //Configure view settings
    view.setRootEntity(rootEntiy);

    view.setActiveFrameGraph(pFramegragh);

    view.renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::Always);



    view.show();


    return a.exec();
}

QByteArray createVertexBufferFromBemSurface(const MNEBemSurface &tBemSurface)
{
    const int iVertNum = tBemSurface.rr.rows();
    const int iVertSize = tBemSurface.rr.cols();

    QByteArray bufferData;
    bufferData.resize(iVertNum * iVertSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    int idxVert = 0;
    for(int i = 0; i < tBemSurface.rr.rows(); ++i) {
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,0));
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,1));
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,2));
    }

    return bufferData;
}

QByteArray createColorBuffer(const int iVertNum)
{
    const int iBufferSize = iVertNum;
    QByteArray bufferData;
    bufferData.resize(iBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(int i = 0; i < iBufferSize; ++i)
    {
        rawVertexArray[i] = 1.0f;
    }
    return bufferData;
}

MatrixX3f createVertColor(const MatrixXf& vertices, const QColor& color)
{
    MatrixX3f matColor(vertices.rows(),3);

    for(int i = 0; i < matColor.rows(); ++i) {
        matColor(i,0) = color.redF();
        matColor(i,1) = color.greenF();
        matColor(i,2) = color.blueF();
    }

    return matColor;
}

QByteArray createWeightMatBuffer(QSharedPointer<SparseMatrix<double> > tInterpolationMatrix)
{
    QByteArray bufferData;

    const uint iRows = tInterpolationMatrix->rows();
    const uint iCols = tInterpolationMatrix->cols();

    bufferData.resize(iRows * iCols * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    unsigned int iCtr = 0;
    for(uint i = 0; i < iRows; ++i)
    {
        for(uint j = 0; j < iCols; ++j)
        {
            //@TODO this is probably not the best way to extract the weight matrix components
            rawVertexArray[iCtr] = tInterpolationMatrix->coeff(i, j);
            iCtr++;
        }
    }

    return bufferData;
}
