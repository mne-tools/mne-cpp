//=============================================================================================================
/**
* @file     noisereduction.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the NoiseReduction class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseReductionPlugin;
using namespace MNEX;
using namespace XMEASLIB;
using namespace UTILSLIB;
using namespace IOBuffer;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReduction::NoiseReduction()
: m_bIsRunning(false)
, m_pNoiseReductionInput(NULL)
, m_pNoiseReductionOutput(NULL)
, m_pNoiseReductionBuffer(CircularMatrixBuffer<double>::SPtr())
, m_bSpharaActive(false)
, m_sCurrentSystem("VectorView")
{
    if(m_sCurrentSystem == "BabyMEG") {
        m_iNBaseFctsFirst = 270;
        m_iNBaseFctsSecond = 105;
    }

    if(m_sCurrentSystem == "VectorView") {
        m_iNBaseFctsFirst = 102;
        m_iNBaseFctsSecond = 102;
    }

    //Create toolbar widgets
    m_pOptionsWidget = NoiseReductionOptionsWidget::SPtr(new NoiseReductionOptionsWidget(this));
    m_pOptionsWidget->setAcquisitionSystem(m_sCurrentSystem);

    //Add action which will be visible in the plugin's toolbar
    m_pActionShowOptionsWidget = new QAction(QIcon(":/images/options.png"), tr("Noise reduction options"),this);
    m_pActionShowOptionsWidget->setShortcut(tr("F12"));
    m_pActionShowOptionsWidget->setStatusTip(tr("Noise reduction options"));
    connect(m_pActionShowOptionsWidget, &QAction::triggered,
            this, &NoiseReduction::showOptionsWidget);
    addPluginAction(m_pActionShowOptionsWidget);
}


//*************************************************************************************************************

NoiseReduction::~NoiseReduction()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> NoiseReduction::clone() const
{
    QSharedPointer<NoiseReduction> pNoiseReductionClone(new NoiseReduction);
    return pNoiseReductionClone;
}


//*************************************************************************************************************

void NoiseReduction::init()
{
    // Input
    m_pNoiseReductionInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "NoiseReductionIn", "NoiseReduction input data");
    connect(m_pNoiseReductionInput.data(), &PluginInputConnector::notify, this, &NoiseReduction::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNoiseReductionInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pNoiseReductionOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "NoiseReductionOut", "NoiseReduction output data");
    m_outputConnectors.append(m_pNoiseReductionOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pNoiseReductionBuffer.isNull())
        m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void NoiseReduction::unload()
{

}


//*************************************************************************************************************

bool NoiseReduction::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool NoiseReduction::stop()
{
    m_bIsRunning = false;

    m_pNoiseReductionBuffer->releaseFromPop();
    m_pNoiseReductionBuffer->clear();

    m_pNoiseReductionBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType NoiseReduction::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString NoiseReduction::getName() const
{
    return "NoiseReduction Toolbox";
}


//*************************************************************************************************************

QWidget* NoiseReduction::setupWidget()
{
    NoiseReductionSetupWidget* setupWidget = new NoiseReductionSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void NoiseReduction::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pNoiseReductionBuffer) {
            m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pNoiseReductionOutput in the constructor above
            m_pNoiseReductionOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pNoiseReductionOutput->data()->setMultiArraySize(1);
            m_pNoiseReductionOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pNoiseReductionBuffer->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void NoiseReduction::setAcquisitionSystem(const QString& sSystem)
{
    m_mutex.lock();
    m_sCurrentSystem = sSystem;
    m_mutex.unlock();

    createSpharaOperator();
}

//*************************************************************************************************************

void NoiseReduction::setSpharaMode(bool state)
{
    m_mutex.lock();
    m_bSpharaActive = state;
    m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::setSpharaNBaseFcts(int nBaseFctsGrad, int nBaseFctsMag)
{
    m_mutex.lock();
    m_iNBaseFctsFirst = nBaseFctsGrad;
    m_iNBaseFctsSecond = nBaseFctsMag;
    m_mutex.unlock();

    createSpharaOperator();
}


//*************************************************************************************************************

void NoiseReduction::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    //Read and create SPHARA operator for the first time
    initSphara();
    createSpharaOperator();

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pNoiseReductionBuffer->pop();

        m_mutex.lock();

        //Do all the noise reduction steps here
        //SPHARA calculations
        if(m_bSpharaActive) {
            t_mat = m_matSpharaMultFirst * m_matSpharaMultSecond * t_mat;
        }

        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pNoiseReductionOutput in the constructor above
        m_pNoiseReductionOutput->data()->setValue(t_mat);
    }
}


//*************************************************************************************************************

void NoiseReduction::showOptionsWidget()
{
    m_pOptionsWidget->show();
}


//*************************************************************************************************************

void NoiseReduction::initSphara()
{
    m_matSpharaMultFirst = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    m_matSpharaMultSecond = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    //Load SPHARA matrix
    read_eigen_matrix(m_matSpharaVVGradLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    read_eigen_matrix(m_matSpharaVVMagLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    //Generate indices used to create the SPHARA operators.
    if(m_sCurrentSystem == "VectorView") {
        indicesFirstVV.resize(0);
        indicesSecondVV.resize(0);

        for(int r = 0; r<m_pFiffInfo->chs.size(); r++) {
            //Find GRADIOMETERS
            if(m_pFiffInfo->chs.at(r).coil_type == 3012) {
                indicesFirstVV.conservativeResize(indicesFirstVV.rows()+1);
                indicesFirstVV(indicesFirstVV.rows()-1) = r;
            }

            //Find Magnetometers
            if(m_pFiffInfo->chs.at(r).coil_type == 3024) {
                indicesSecondVV.conservativeResize(indicesSecondVV.rows()+1);
                indicesSecondVV(indicesSecondVV.rows()-1) = r;
            }
        }
    }

    if(m_sCurrentSystem == "BabyMEG") {
        indicesFirstBabyMEG.resize(0);
        for(int r = 0; r<m_pFiffInfo->chs.size(); r++) {
            //Find INNER LAYER
            if(m_pFiffInfo->chs.at(r).coil_type == 7002) {
                indicesFirstBabyMEG.conservativeResize(indicesFirstBabyMEG.rows()+1);
                indicesFirstBabyMEG(indicesFirstBabyMEG.rows()-1) = r;
            }

            //TODO: Find outer layer
        }
    }

//    qDebug()<<"NoiseReduction::createSpharaOperator - Read VectorView mag matrix "<<m_matSpharaVVMagLoaded.rows()<<m_matSpharaVVMagLoaded.cols()<<"and grad matrix"<<m_matSpharaVVGradLoaded.rows()<<m_matSpharaVVGradLoaded.cols();
//    qDebug()<<"NoiseReduction::createSpharaOperator - Read BabyMEG inner layer matrix "<<m_matSpharaBabyMEGInnerLoaded.rows()<<m_matSpharaBabyMEGInnerLoaded.cols()<<"and outer layer matrix"<<m_matSpharaBabyMEGOuterFull.rows()<<m_matSpharaBabyMEGOuterFull.cols();
}


//*************************************************************************************************************

void NoiseReduction::createSpharaOperator()
{
    qDebug()<<"NoiseReduction::createSpharaOperator - Creating SPHARA oerpator for"<<m_sCurrentSystem;

    m_mutex.lock();

    if(m_sCurrentSystem == "VectorView") {
        m_matSpharaMultFirst = Sphara::makeSpharaProjector(m_matSpharaVVGradLoaded, indicesFirstVV, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 1); //GRADIOMETERS
        m_matSpharaMultSecond = Sphara::makeSpharaProjector(m_matSpharaVVMagLoaded, indicesSecondVV, m_pFiffInfo->nchan, m_iNBaseFctsSecond, 0); //Magnetometers
    }

    if(m_sCurrentSystem == "BabyMEG") {
        m_matSpharaMultFirst = Sphara::makeSpharaProjector(m_matSpharaBabyMEGInnerLoaded, indicesFirstBabyMEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    //Write final operator matrices to file
    write_eigen_matrix(m_matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/m_matSpharaMultFirst.txt"));
    write_eigen_matrix(m_matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/m_matSpharaMultSecond.txt"));

    m_mutex.unlock();
}


//*************************************************************************************************************

template<typename T>
void NoiseReduction::read_eigen_matrix(Matrix<T, Dynamic, Dynamic>& out, const QString& path)
{
    QFile file(path);

    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //Start reading from file
        QTextStream in(&file);
        int i=0;
        QList<VectorXd> help;

        while(!in.atEnd())
        {
            QString line = in.readLine();
            QStringList fields = line.split(QRegExp("\\s+"));

            VectorXd x (fields.size());
            //Read actual electrode position
            for (int j=0; j<fields.size(); j++) {
                x(j)=fields.at(j).toDouble();
            }

            help.append(x);

            i++;
        }

        int rows = help.size();
        int cols = rows<=0 ? 0 : help.at(0).rows();

        out.resize(rows, cols);

        for (int i=0; i<help.length(); i++) {
            out.row(i) = help[i].transpose();
        }
    } else {
        qWarning()<<"IOUtils::read_eigen_matrix - Could not read Eigen element from file! Path does not exist!";
    }
}



//*************************************************************************************************************

template<typename T>
void NoiseReduction::write_eigen_matrix(const Matrix<T, Dynamic, Dynamic>& in, const QString& path)
{
    QFile file(path);
    if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        QTextStream stream(&file);
        stream<<"Dimensions (rows x cols): "<<in.rows()<<" x "<<in.cols()<<"\n";
        for(int row = 0; row<in.rows(); row++) {
            for(int col = 0; col<in.cols(); col++)
                stream << in(row, col)<<" ";
            stream<<"\n";
        }
    } else {
        qWarning()<<"Could not write Eigen element to file! Path does not exist!";
    }

    file.close();
}

