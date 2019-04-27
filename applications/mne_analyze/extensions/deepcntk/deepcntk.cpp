//=============================================================================================================
/**
* @file     deepcntk.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the DeepCNTK class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepcntk.h"
#include "IDeepCNTKNet.h"
#include "deepcntkmanager.h"

#include <deep/deep.h>
#include <deep/deepmodelcreator.h>

#include <disp/lineplot.h>
#include <disp/deepmodelviewer/deepviewer.h>
#include <disp/deepmodelviewer/controls.h>

#include <iostream>
#include <random>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QComboBox>


//*************************************************************************************************************
//=============================================================================================================
// CNTK INCLUDES
//=============================================================================================================

#include <CNTKLibrary.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DEEPCNTKEXTENSION;
using namespace ANSHAREDLIB;
using namespace Eigen;
using namespace DEEPLIB;
using namespace DISPLIB;
//using namespace QtCharts;
using namespace CNTK;


//*************************************************************************************************************
//=============================================================================================================
// CONST
//=============================================================================================================

const char* deepCNTKNetsDir = "/mne_analyze_extensions/deepcntknets";        /**< holds path to the extensions.*/


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
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepCNTK::DeepCNTK()
: m_pControlPanel(Q_NULLPTR)
, m_pControl(Q_NULLPTR)
, m_pDeepViewer(Q_NULLPTR)
, m_pDeepCNTKManager(Q_NULLPTR)
{

}


//*************************************************************************************************************

DeepCNTK::~DeepCNTK()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> DeepCNTK::clone() const
{
    QSharedPointer<DeepCNTK> pDeepCNTKClone(new DeepCNTK);
    return pDeepCNTKClone;
}


//*************************************************************************************************************

void DeepCNTK::init()
{
    //
    // Deep Configuration Manager
    //
    if(!m_pDeepCNTKManager) {
        m_pDeepCNTKManager = new DeepCNTKManager(this);
        m_pDeepCNTKManager->loadDeepConfigurations(qApp->applicationDirPath()+deepCNTKNetsDir);
        m_pDeepCNTKManager->initDeepConfigurations();
    }

    //
    // Init view
    //
    if(!m_pControlPanel) {
        m_pControlPanel = new Controls;
    }

    //
    // Create the viewer
    //
    if(!m_pDeepViewer) {
        qDebug() << "m_pDeepCNTKManager->currentDeepConfiguration()" << m_pDeepCNTKManager->currentDeepConfiguration();
        m_pDeepCNTKManager->currentDeepConfiguration()->getModel()->print();

        m_pDeepViewer = new DeepViewer(false);

        if(m_pDeepCNTKManager->currentDeepConfiguration()->getName() != "BIO") {
            // Don't display model when its too complex
            m_pDeepViewer->setModel(m_pDeepCNTKManager->currentDeepConfiguration()->getModel());
        }


        m_pControlPanel->setDeepViewer(m_pDeepViewer);
        m_pDeepViewer->setWindowTitle("Deep CNTK");
    }

    QComboBox *combo = m_pControlPanel->getConfigurationComboBox();
    combo->addItems(m_pDeepCNTKManager->getDeepConfigurationNames());


    connect(m_pControlPanel, &Controls::requestTraining_signal, m_pDeepCNTKManager, &DeepCNTKManager::trainCurrentConfiguration);
    connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_pDeepCNTKManager, &DeepCNTKManager::selectDeepConfiguration);


    connect(m_pDeepCNTKManager, &DeepCNTKManager::finishedTraining_signal, this, &DeepCNTK::trainingFinished);
    connect(m_pDeepCNTKManager, &DeepCNTKManager::currentConfigurationChanged_signal, this, &DeepCNTK::resetDeepViewer);

}


//*************************************************************************************************************

void DeepCNTK::unload()
{

}


//*************************************************************************************************************

QString DeepCNTK::getName() const
{
    return "Deep CNTK";
}


//*************************************************************************************************************

QMenu *DeepCNTK::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *DeepCNTK::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("Deep CNTK"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControl->setMinimumWidth(180);
        m_pControl->setWidget(m_pControlPanel);
    }

    return m_pControl;
}


//*************************************************************************************************************

// check with owner ship and mdi area for garbage collection
QWidget *DeepCNTK::getView()
{
    return m_pDeepViewer;
}


//*************************************************************************************************************

void DeepCNTK::trainingFinished()
{
    QString configName = m_pDeepCNTKManager->currentDeepConfiguration()->getName();
    //Plot error
    LinePlot *error_chartView = new LinePlot(m_pDeepCNTKManager->currentDeepConfiguration()->currentError(),QString("%1: Current Training Error").arg(configName));
    error_chartView->show();

    //Plot loss
    LinePlot *loss_chartView = new LinePlot(m_pDeepCNTKManager->currentDeepConfiguration()->currentLoss(),QString("%1: Current Training Loss").arg(configName));
    loss_chartView->show();

   // Update Deep Viewer
    updateDeepViewer();
}


//*************************************************************************************************************

void DeepCNTK::resetDeepViewer()
{
    m_pDeepViewer->setModel(m_pDeepCNTKManager->currentDeepConfiguration()->getModel());
}


//*************************************************************************************************************

void DeepCNTK::updateDeepViewer()
{
    qDebug() << "void DeepCNTK::modelUpdated()";

   // Update Deep Viewer
    if(m_pDeepViewer) {
        m_pDeepViewer->updateModel();
    }
}
