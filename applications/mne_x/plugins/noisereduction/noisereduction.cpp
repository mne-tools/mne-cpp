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
, m_bProjActivated(false)
, m_bCompActivated(false)
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

    //Handle projections
    connect(m_pOptionsWidget.data(), &NoiseReductionOptionsWidget::projSelectionChanged,
            this, &NoiseReduction::updateProjection);

    //Add action which will be visible in the plugin's toolbar
    m_pActionShowOptionsWidget = new QAction(QIcon(":/images/options.png"), tr("Noise reduction options"),this);
    m_pActionShowOptionsWidget->setShortcut(tr("F12"));
    m_pActionShowOptionsWidget->setStatusTip(tr("Noise reduction options"));
    connect(m_pActionShowOptionsWidget, &QAction::triggered,
            this, &NoiseReduction::showOptionsWidget);
    addPluginAction(m_pActionShowOptionsWidget);
    m_pActionShowOptionsWidget->setVisible(false);
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
    connect(m_pNoiseReductionInput.data(), &PluginInputConnector::notify,
            this, &NoiseReduction::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNoiseReductionInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    m_pNoiseReductionOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "NoiseReductionOut", "NoiseReduction output data");
    m_outputConnectors.append(m_pNoiseReductionOutput);

    QStringList slFlags;
    slFlags << "view" << "triggerdetection" << "scaling";
    m_pNoiseReductionOutput->data()->setDisplayFlags(slFlags);

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

            //Init the multiplication matrices
            m_matSparseProj = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseComp = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseSpharaMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseSpharaProjMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseSpharaCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseProjCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseFull = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

            m_matSparseProj.setIdentity();
            m_matSparseComp.setIdentity();
            m_matSparseSpharaMult.setIdentity();
            m_matSparseSpharaProjMult.setIdentity();
            m_matSparseSpharaCompMult.setIdentity();
            m_matSparseProjCompMult.setIdentity();
            m_matSparseFull.setIdentity();

            m_pOptionsWidget->setFiffInfo(m_pFiffInfo);

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

void NoiseReduction::updateProjection()
{
    //
    //  Update the SSP projector
    //
    if(m_pFiffInfo)
    {
        m_mutex.lock();
        //If a minimum of one projector is active set m_bProjActivated to true so that this model applies the ssp to the incoming data
        m_bProjActivated = false;
        for(qint32 i = 0; i < this->m_pFiffInfo->projs.size(); ++i) {
            if(this->m_pFiffInfo->projs[i].active) {
                m_bProjActivated = true;
                break;
            }
        }

        MatrixXd matProj;
        this->m_pFiffInfo->make_projector(matProj);
        qDebug() << "NoiseReduction::updateProjection - New projection calculated.";
        qDebug() << "NoiseReduction::updateProjection - m_bProjActivated:"<<m_bProjActivated;

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_pFiffInfo->bads.size(); ++j) {
            int index = m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(j));
            if(index >= 0 && index<m_pFiffInfo->ch_names.size()) {
                matProj.col(index).setZero();
            }
        }

//        std::cout << "Bads\n" << m_vecBadIdcs << std::endl;
//        std::cout << "Proj\n";
//        std::cout << matProj.block(0,0,10,10) << std::endl;

        //
        // Make proj sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        tripletList.clear();
        tripletList.reserve(matProj.rows()*matProj.cols());
        for(i = 0; i < matProj.rows(); ++i)
            for(k = 0; k < matProj.cols(); ++k)
                if(matProj(i,k) != 0)
                    tripletList.push_back(T(i, k, matProj(i,k)));

        m_matSparseProj = SparseMatrix<double>(matProj.rows(),matProj.cols());
        if(tripletList.size() > 0)
            m_matSparseProj.setFromTriplets(tripletList.begin(), tripletList.end());

        //Create full multiplication matrix
        m_matSparseSpharaProjMult = m_matSparseSpharaMult * m_matSparseProj;
        m_matSparseProjCompMult = m_matSparseProj * m_matSparseComp;

        m_matSparseFull = m_matSparseSpharaMult * m_matSparseProj * m_matSparseComp;
        m_mutex.unlock();
    }
}


//*************************************************************************************************************

void NoiseReduction::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    //Set visibility of options tool to true
    m_pActionShowOptionsWidget->setVisible(true);

    //Read and create SPHARA operator for the first time
    initSphara();
    createSpharaOperator();

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pNoiseReductionBuffer->pop();

        m_mutex.lock();

        //Do all the noise reduction steps here
        if(m_bCompActivated) {
            if(m_bProjActivated) {
                if(m_bSpharaActive) {
                    //Comp + Proj + Sphara
                    t_mat = m_matSparseFull * t_mat;
                } else {
                    //Comp + Proj
                    t_mat = m_matSparseProjCompMult * t_mat;
                }
            } else {
                if(m_bSpharaActive) {
                    //Comp + Sphara
                    t_mat = m_matSparseSpharaCompMult * t_mat;
                } else {
                    //Comp
                    t_mat = m_matSparseComp * t_mat;
                }
            }
        } else {
            if(m_bProjActivated) {
                if(m_bSpharaActive) {
                    //Proj + Sphara
                    t_mat = m_matSparseSpharaProjMult * t_mat;
                } else {
                    //Proj
                    t_mat = m_matSparseProj * t_mat;
                }
            } else {
                if(m_bSpharaActive) {
                    //Sphara
                    t_mat = m_matSparseSpharaMult * t_mat;
                } else {
                    //None - Raw
                }
            }
        }

        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
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
    //Load SPHARA matrix
    IOUtils::read_eigen_matrix(m_matSpharaVVGradLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaVVMagLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    //Generate indices used to create the SPHARA operators.
    m_vecIndicesFirstVV.resize(0);
    m_vecIndicesSecondVV.resize(0);

    for(int r = 0; r<m_pFiffInfo->chs.size(); r++) {
        //Find GRADIOMETERS
        if(m_pFiffInfo->chs.at(r).coil_type == 3012) {
            m_vecIndicesFirstVV.conservativeResize(m_vecIndicesFirstVV.rows()+1);
            m_vecIndicesFirstVV(m_vecIndicesFirstVV.rows()-1) = r;
        }

        //Find Magnetometers
        if(m_pFiffInfo->chs.at(r).coil_type == 3024) {
            m_vecIndicesSecondVV.conservativeResize(m_vecIndicesSecondVV.rows()+1);
            m_vecIndicesSecondVV(m_vecIndicesSecondVV.rows()-1) = r;
        }
    }


    m_vecIndicesFirstBabyMEG.resize(0);
    for(int r = 0; r<m_pFiffInfo->chs.size(); r++) {
        //Find INNER LAYER
        if(m_pFiffInfo->chs.at(r).coil_type == 7002) {
            m_vecIndicesFirstBabyMEG.conservativeResize(m_vecIndicesFirstBabyMEG.rows()+1);
            m_vecIndicesFirstBabyMEG(m_vecIndicesFirstBabyMEG.rows()-1) = r;
        }

        //TODO: Find outer layer
    }

//    qDebug()<<"NoiseReduction::createSpharaOperator - Read VectorView mag matrix "<<m_matSpharaVVMagLoaded.rows()<<m_matSpharaVVMagLoaded.cols()<<"and grad matrix"<<m_matSpharaVVGradLoaded.rows()<<m_matSpharaVVGradLoaded.cols();
//    qDebug()<<"NoiseReduction::createSpharaOperator - Read BabyMEG inner layer matrix "<<m_matSpharaBabyMEGInnerLoaded.rows()<<m_matSpharaBabyMEGInnerLoaded.cols()<<"and outer layer matrix"<<m_matSpharaBabyMEGOuterFull.rows()<<m_matSpharaBabyMEGOuterFull.cols();
}


//*************************************************************************************************************

void NoiseReduction::createSpharaOperator()
{
    qDebug()<<"NoiseReduction::createSpharaOperator - Creating SPHARA oerpator for"<<m_sCurrentSystem;

    m_mutex.lock();

    MatrixXd matSpharaMultFirst = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    MatrixXd matSpharaMultSecond = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    if(m_sCurrentSystem == "VectorView") {
        matSpharaMultFirst = Sphara::makeSpharaProjector(m_matSpharaVVGradLoaded, m_vecIndicesFirstVV, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 1); //GRADIOMETERS
        matSpharaMultSecond = Sphara::makeSpharaProjector(m_matSpharaVVMagLoaded, m_vecIndicesSecondVV, m_pFiffInfo->nchan, m_iNBaseFctsSecond, 0); //Magnetometers
    }

    if(m_sCurrentSystem == "BabyMEG") {
        matSpharaMultFirst = Sphara::makeSpharaProjector(m_matSpharaBabyMEGInnerLoaded, m_vecIndicesFirstBabyMEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    //Write final operator matrices to file
    IOUtils::write_eigen_matrix(matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMultFirst.txt"));
    IOUtils::write_eigen_matrix(matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "/mne_x_plugins/resources/noisereduction/SPHARA/matSpharaMultSecond.txt"));

    //
    // Make operators sparse
    //
    qint32 nchan = this->m_pFiffInfo->nchan;
    qint32 i, k;

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);

    //First operator
    tripletList.clear();
    tripletList.reserve(matSpharaMultFirst.rows()*matSpharaMultFirst.cols());
    for(i = 0; i < matSpharaMultFirst.rows(); ++i)
        for(k = 0; k < matSpharaMultFirst.cols(); ++k)
            if(matSpharaMultFirst(i,k) != 0)
                tripletList.push_back(T(i, k, matSpharaMultFirst(i,k)));

    Eigen::SparseMatrix<double> matSparseSpharaMultFirst = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0)
        matSparseSpharaMultFirst.setFromTriplets(tripletList.begin(), tripletList.end());

    //Second operator
    tripletList.clear();
    tripletList.reserve(matSpharaMultSecond.rows()*matSpharaMultSecond.cols());

    for(i = 0; i < matSpharaMultSecond.rows(); ++i)
        for(k = 0; k < matSpharaMultSecond.cols(); ++k)
            if(matSpharaMultSecond(i,k) != 0)
                tripletList.push_back(T(i, k, matSpharaMultSecond(i,k)));

    Eigen::SparseMatrix<double>matSparseSpharaMultSecond = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0)
        matSparseSpharaMultSecond.setFromTriplets(tripletList.begin(), tripletList.end());

    //Create full multiplication matrix
    m_matSparseSpharaMult = matSparseSpharaMultFirst * matSparseSpharaMultSecond;
    m_matSparseSpharaProjMult = m_matSparseSpharaMult * m_matSparseProj;
    m_matSparseSpharaCompMult = m_matSparseSpharaMult * m_matSparseComp;

    m_matSparseFull = m_matSparseSpharaMult * m_matSparseProj * m_matSparseComp;

    m_mutex.unlock();
}
