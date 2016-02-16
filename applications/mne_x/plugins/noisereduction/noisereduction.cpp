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
, m_iNBaseFcts(102)
, m_bSpharaActive(false)
, m_sCurrentSystem("VectorView")
{
    //Create toolbar widgets
    m_pOptionsWidget = NoiseReductionOptionsWidget::SPtr(new NoiseReductionOptionsWidget(this));

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

void NoiseReduction::setSpharaMode(bool state)
{
    m_mutex.lock();
    m_bSpharaActive = state;
    m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::setSpharaNBaseFcts(int nBaseFcts)
{
    m_mutex.lock();
    m_iNBaseFcts = nBaseFcts;

    creatSpharaOperator();

    m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    //create SPHARA operator
    creatSpharaOperator();

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pNoiseReductionBuffer->pop();

        m_mutex.lock();

        //Do all the noise reduction steps here
        //SPHARA calculations
        if(m_bSpharaActive) {
            t_mat = m_matSpharaMultGrad * t_mat;
            t_mat = m_matSpharaMultMag * t_mat;
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

void NoiseReduction::creatSpharaOperator()
{
    m_mutex.lock();

    m_matSpharaMultGrad = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    m_matSpharaMultMag = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    if(m_sCurrentSystem == "VectorView") {
        //Read SPHARA matrix
        MatrixXd matSpharaGradFull;
        read_eigen_matrix(matSpharaGradFull, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
        MatrixXd matSpharaMagFull;
        read_eigen_matrix(matSpharaMagFull, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

        qDebug()<<"NoiseReduction::creatSpharaOperator - Read VectorView mag matrix "<<matSpharaMagFull.rows()<<matSpharaMagFull.cols()<<"and grad matrix"<<matSpharaGradFull.rows()<<matSpharaGradFull.cols();

        //GRADIOMETERS
        //Remove unwanted base functions
        qDebug()<<"NoiseReduction::creatSpharaOperator - Removing"<<matSpharaGradFull.rows() - m_iNBaseFcts<<"SPHARA grad base functions";

        MatrixXd matSpharaGradCut = MatrixXd::Zero(matSpharaGradFull.rows(), matSpharaGradFull.cols());
        matSpharaGradCut.block(0,0,m_iNBaseFcts,m_iNBaseFcts) = matSpharaGradFull.block(0,0,m_iNBaseFcts,m_iNBaseFcts);
        MatrixXd matSpharaMultGrad = matSpharaGradCut * matSpharaGradFull.transpose();

        //Create full gradiometer SPHARA operator
        for(int r = 0; r<matSpharaMultGrad.rows(); r++) {
            for(int c = 0; c<matSpharaMultGrad.cols(); c++) {
                MatrixXd triplet(3,3);
                if(r != c) {
                    triplet <<  matSpharaMultGrad(r,c), 0, 0,
                                0, matSpharaMultGrad(r,c), 0,
                                0, 0, 0;
                } else {
                    triplet <<  matSpharaMultGrad(r,c), 0, 0,
                                0, matSpharaMultGrad(r,c), 0,
                                0, 0, 1;
                }

                m_matSpharaMultGrad.block(r*3,c*3,3,3) = triplet;
            }
        }

        //Write final operator matrices to file
        write_eigen_matrix(matSpharaGradFull, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaGradFull.txt"));
        write_eigen_matrix(matSpharaGradCut, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaGradCut.txt"));
        write_eigen_matrix(matSpharaMultGrad, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMultGrad.txt"));
        write_eigen_matrix(m_matSpharaMultGrad, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/m_matSpharaMultGrad.txt"));

        //Magnetometers
        //Remove unwanted base functions
        qDebug()<<"NoiseReduction::creatSpharaOperator - Removing"<<matSpharaMagFull.rows() - m_iNBaseFcts<<"SPHARA mag base functions";

        MatrixXd matSpharaMagCut = MatrixXd::Identity(matSpharaMagFull.rows(), matSpharaMagFull.cols());
        matSpharaMagCut.block(0,0,m_iNBaseFcts,m_iNBaseFcts) = matSpharaMagFull.block(0,0,m_iNBaseFcts,m_iNBaseFcts);
        MatrixXd matSpharaMultMag = matSpharaMagCut * matSpharaMagFull.transpose();

        //Create full magnetometer SPHARA operator
        for(int r = 0; r<matSpharaMultMag.rows(); r++) {
            for(int c = 0; c<matSpharaMultMag.cols(); c++) {
                MatrixXd triplet(3,3);
                if(r != c) {
                    triplet <<  0, 0, 0,
                                0, 0, 0,
                                0, 0, matSpharaMultMag(r,c);
                } else {
                    triplet <<  1, 0, 0,
                                0, 1, 0,
                                0, 0, matSpharaMultMag(r,c);
                }

                m_matSpharaMultMag.block(r*3,c*3,3,3) = triplet;
            }
        }

        //Write final operator matrices to file
        write_eigen_matrix(matSpharaMagFull, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMagFull.txt"));
        write_eigen_matrix(matSpharaMagCut, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMagCut.txt"));
        write_eigen_matrix(matSpharaMultMag, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMultMag.txt"));
        write_eigen_matrix(m_matSpharaMultMag, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/m_matSpharaMultMag.txt"));
    }

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

