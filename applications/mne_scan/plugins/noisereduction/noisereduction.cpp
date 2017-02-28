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
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace IOBUFFER;
using namespace Eigen;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReduction::NoiseReduction()
: m_bIsRunning(false)
, m_pNoiseReductionInput(NULL)
, m_pNoiseReductionOutput(NULL)
, m_pNoiseReductionBuffer(CircularMatrixBuffer<double>::SPtr())
, m_iMaxFilterTapSize(0)
, m_bSpharaActive(false)
, m_bFilterActivated(false)
, m_bProjActivated(false)
, m_bCompActivated(false)
, m_sCurrentSystem("VectorView")
, m_pRTMSA(NewRealTimeMultiSampleArray::SPtr(new NewRealTimeMultiSampleArray()))
, m_pFilterWindow(Q_NULLPTR)
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
    m_pActionShowOptionsWidget->setVisible(false);
}


//*************************************************************************************************************

NoiseReduction::~NoiseReduction()
{
    if(this->isRunning()) {
        stop();
    }

    //
    // Store Settings
    //
    if(!m_pRTMSA->getName().isEmpty()) {
        QString t_sRTMSAName = m_pRTMSA->getName();

        QSettings settings;

        //Store filter
        if(m_pFilterWindow != 0) {
            FilterData filter = m_pFilterWindow->getUserDesignedFilter();

            settings.setValue(QString("RTNRW/%1/filterHP").arg(t_sRTMSAName), filter.m_dHighpassFreq);
            settings.setValue(QString("RTNRW/%1/filterLP").arg(t_sRTMSAName), filter.m_dLowpassFreq);
            settings.setValue(QString("RTNRW/%1/filterOrder").arg(t_sRTMSAName), filter.m_iFilterOrder);
            settings.setValue(QString("RTNRW/%1/filterType").arg(t_sRTMSAName), (int)filter.m_Type);
            settings.setValue(QString("RTNRW/%1/filterDesignMethod").arg(t_sRTMSAName), (int)filter.m_designMethod);
            settings.setValue(QString("RTNRW/%1/filterTransition").arg(t_sRTMSAName), filter.m_dParksWidth*(filter.m_sFreq/2));
            settings.setValue(QString("RTNRW/%1/filterUserDesignActive").arg(t_sRTMSAName), m_pFilterWindow->userDesignedFiltersIsActive());
            settings.setValue(QString("RTNRW/%1/filterChannelType").arg(t_sRTMSAName), m_pFilterWindow->getChannelType());
        }
    }
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
    slFlags << "view" << "triggerdetection" << "scaling" << "colors";
    m_pNoiseReductionOutput->data()->setDisplayFlags(slFlags);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pNoiseReductionBuffer.isNull())
        m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr();

    //Handle projections
    connect(m_pOptionsWidget.data(), &NoiseReductionOptionsWidget::projSelectionChanged,
            this, &NoiseReduction::updateProjection);

    //Handle compensators
    connect(m_pOptionsWidget.data(), &NoiseReductionOptionsWidget::compSelectionChanged,
            this, &NoiseReduction::updateCompensator);
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
    return "NoiseReduction";
}


//*************************************************************************************************************

QWidget* NoiseReduction::setupWidget()
{
    NoiseReductionSetupWidget* setupWidget = new NoiseReductionSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void NoiseReduction::update(SCMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    //QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    m_pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(m_pRTMSA) {
        //Check if buffer initialized
        if(!m_pNoiseReductionBuffer) {
            m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, m_pRTMSA->getNumChannels(), m_pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = m_pRTMSA->info();

            //Init the multiplication matrices
            m_matSparseProjMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseSpharaMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseProjCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseFull = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

            m_matSparseProjMult.setIdentity();
            m_matSparseCompMult.setIdentity();
            m_matSparseSpharaMult.setIdentity();
            m_matSparseProjCompMult.setIdentity();
            m_matSparseFull.setIdentity();

            m_pOptionsWidget->setFiffInfo(m_pFiffInfo);

            //Init output - Unocmment this if you also uncommented the m_pNoiseReductionOutput in the constructor above
            m_pNoiseReductionOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pNoiseReductionOutput->data()->setMultiArraySize(1);
            m_pNoiseReductionOutput->data()->setVisibility(true);            

            //Init the filter
            m_iMaxFilterTapSize = m_pRTMSA->getMultiSampleArray().at(m_pRTMSA->getMultiSampleArray().size()-1).cols();
            initFilter();
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < m_pRTMSA->getMultiArraySize(); ++i) {
            t_mat = m_pRTMSA->getMultiSampleArray()[i];
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
    if(m_pFiffInfo) {
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
//        qDebug() << "NoiseReduction::updateProjection - New projection calculated.";
//        qDebug() << "NoiseReduction::updateProjection - m_bProjActivated:"<<m_bProjActivated;

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
        for(i = 0; i < matProj.rows(); ++i) {
            for(k = 0; k < matProj.cols(); ++k) {
                if(matProj(i,k) != 0) {
                    tripletList.push_back(T(i, k, matProj(i,k)));
                }
            }
        }

        m_matSparseProjMult = SparseMatrix<double>(matProj.rows(),matProj.cols());
        if(tripletList.size() > 0)
            m_matSparseProjMult.setFromTriplets(tripletList.begin(), tripletList.end());

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;

        m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;
        m_mutex.unlock();
    }
}


//*************************************************************************************************************

void NoiseReduction::updateCompensator(int to)
{
    //
    //  Update the compensator
    //
    if(m_pFiffInfo)
    {
        if(to == 0) {
            m_bCompActivated = false;
        } else {
            m_bCompActivated = true;
        }

//        qDebug()<<"to"<<to;
//        qDebug()<<"from"<<from;
//        qDebug()<<"m_bCompActivated"<<m_bCompActivated;

        FiffCtfComp newComp;
        this->m_pFiffInfo->make_compensator(0, to, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data

        this->m_pFiffInfo->set_current_comp(to);
        MatrixXd matComp = newComp.data->data;

        //
        // Make proj sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        tripletList.clear();
        tripletList.reserve(matComp.rows()*matComp.cols());
        for(i = 0; i < matComp.rows(); ++i) {
            for(k = 0; k < matComp.cols(); ++k) {
                if(matComp(i,k) != 0) {
                    tripletList.push_back(T(i, k, matComp(i,k)));
                }
            }
        }

        m_matSparseCompMult = SparseMatrix<double>(matComp.rows(),matComp.cols());
        if(tripletList.size() > 0) {
            m_matSparseCompMult.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;

        m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;
    }
}


//*************************************************************************************************************

void NoiseReduction::setFilterChannelType(QString sType)
{
    m_sFilterChannelType = sType;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_lFilterChannelList.clear();

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {

            if(m_sFilterChannelType == "All") {
                m_lFilterChannelList << i;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_lFilterChannelList << i;
            }
        }
    }
}


//*************************************************************************************************************

void NoiseReduction::filterChanged(QList<FilterData> filterData)
{
    m_filterData = filterData;

    m_iMaxFilterLength = 1;
    for(int i = 0; i < filterData.size(); ++i) {
        if(m_iMaxFilterLength<filterData.at(i).m_iFilterOrder) {
            m_iMaxFilterLength = filterData.at(i).m_iFilterOrder;
        }
    }
}


//*************************************************************************************************************

void NoiseReduction::filterActivated(bool state)
{
    m_bFilterActivated = state;
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
    IOUtils::read_eigen_matrix(m_matSpharaVVGradLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaVVMagLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaEEGLoaded, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/Current_SPHARA_EEG.txt"));

    //Generate indices used to create the SPHARA operators for VectorView
    m_vecIndicesFirstVV.resize(0);
    m_vecIndicesSecondVV.resize(0);

    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find GRADIOMETERS
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3012) {
            m_vecIndicesFirstVV.conservativeResize(m_vecIndicesFirstVV.rows()+1);
            m_vecIndicesFirstVV(m_vecIndicesFirstVV.rows()-1) = r;
        }

        //Find Magnetometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3024) {
            m_vecIndicesSecondVV.conservativeResize(m_vecIndicesSecondVV.rows()+1);
            m_vecIndicesSecondVV(m_vecIndicesSecondVV.rows()-1) = r;
        }
    }

    //Generate indices used to create the SPHARA operators for babyMEG
    m_vecIndicesFirstBabyMEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find INNER LAYER
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 7002) {
            m_vecIndicesFirstBabyMEG.conservativeResize(m_vecIndicesFirstBabyMEG.rows()+1);
            m_vecIndicesFirstBabyMEG(m_vecIndicesFirstBabyMEG.rows()-1) = r;
        }

        //TODO: Find outer layer
    }

    //Generate indices used to create the SPHARA operators for EEG layouts
    m_vecIndicesFirstEEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find EEG
        if(m_pFiffInfo->chs.at(r).kind == FIFFV_EEG_CH) {
            m_vecIndicesFirstEEG.conservativeResize(m_vecIndicesFirstEEG.rows()+1);
            m_vecIndicesFirstEEG(m_vecIndicesFirstEEG.rows()-1) = r;
        }
    }

//    qDebug()<<"NoiseReduction::createSpharaOperator - Read VectorView mag matrix "<<m_matSpharaVVMagLoaded.rows()<<m_matSpharaVVMagLoaded.cols()<<"and grad matrix"<<m_matSpharaVVGradLoaded.rows()<<m_matSpharaVVGradLoaded.cols();
//    qDebug()<<"NoiseReduction::createSpharaOperator - Read BabyMEG inner layer matrix "<<m_matSpharaBabyMEGInnerLoaded.rows()<<m_matSpharaBabyMEGInnerLoaded.cols()<<"and outer layer matrix"<<m_matSpharaBabyMEGOuterFull.rows()<<m_matSpharaBabyMEGOuterFull.cols();
}


//*************************************************************************************************************

void NoiseReduction::initFilter()
{
    m_pFilterWindow = FilterWindow::SPtr(new FilterWindow(m_pOptionsWidget.data(), Qt::Window/*0, Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint*/));
    //m_pFilterWindow->setWindowFlags(Qt::WindowStaysOnTopHint);

    m_pFilterWindow->setFiffInfo(m_pFiffInfo);
    m_pFilterWindow->setWindowSize(m_iMaxFilterTapSize);
    m_pFilterWindow->setMaxFilterTaps(m_iMaxFilterTapSize);

    connect(m_pFilterWindow.data(), static_cast<void (FilterWindow::*)(QString)>(&FilterWindow::applyFilter),
            this, static_cast<void (NoiseReduction::*)(QString)>(&NoiseReduction::setFilterChannelType));

    connect(m_pFilterWindow.data(), &FilterWindow::filterChanged,
            this, &NoiseReduction::filterChanged);

    connect(m_pFilterWindow.data(), &FilterWindow::filterActivated,
            this, &NoiseReduction::filterActivated);

    //Handle Filtering
    connect(m_pFilterWindow.data(), &FilterWindow::activationCheckBoxListChanged,
            m_pOptionsWidget.data(), &NoiseReductionOptionsWidget::filterGroupChanged);

    connect(m_pOptionsWidget.data(), &NoiseReductionOptionsWidget::showFilterOptions,
            this, &NoiseReduction::showFilterWidget);

    m_pOptionsWidget->filterGroupChanged(m_pFilterWindow->getActivationCheckBoxList());

    m_pRtFilter = RtFilter::SPtr(new RtFilter());

    this->setFilterChannelType("MEG");

    //Set stored filter settings from last session
    QString t_sRTMSAName = m_pRTMSA->getName();
    QSettings settings;
    m_pFilterWindow->setFilterParameters(settings.value(QString("RTNRW/%1/filterHP").arg(t_sRTMSAName), 5.0).toDouble(),
                                            settings.value(QString("RTNRW/%1/filterLP").arg(t_sRTMSAName), 40.0).toDouble(),
                                            settings.value(QString("RTNRW/%1/filterOrder").arg(t_sRTMSAName), 128).toInt(),
                                            settings.value(QString("RTNRW/%1/filterType").arg(t_sRTMSAName), 2).toInt(),
                                            settings.value(QString("RTNRW/%1/filterDesignMethod").arg(t_sRTMSAName), 0).toInt(),
                                            settings.value(QString("RTNRW/%1/filterTransition").arg(t_sRTMSAName), 5.0).toDouble(),
                                            settings.value(QString("RTNRW/%1/filterUserDesignActive").arg(t_sRTMSAName), false).toBool(),
                                            settings.value(QString("RTNRW/%1/filterChannelType").arg(t_sRTMSAName), "MEG").toString());
}


//*************************************************************************************************************

void NoiseReduction::showFilterWidget(bool state)
{
    if(state) {
        if(m_pFilterWindow->isActiveWindow()) {
            m_pFilterWindow->hide();
        } else {
            m_pFilterWindow->activateWindow();
            m_pFilterWindow->show();
        }
    } else {
        m_pFilterWindow->hide();
    }
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

    if(m_sCurrentSystem == "EEG") {
        matSpharaMultFirst = Sphara::makeSpharaProjector(m_matSpharaEEGLoaded, m_vecIndicesFirstEEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    //Write final operator matrices to file
//    IOUtils::write_eigen_matrix(matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/matSpharaMultFirst.txt"));
//    IOUtils::write_eigen_matrix(matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/resources/noisereduction/SPHARA/matSpharaMultSecond.txt"));

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
    for(i = 0; i < matSpharaMultFirst.rows(); ++i) {
        for(k = 0; k < matSpharaMultFirst.cols(); ++k) {
            if(matSpharaMultFirst(i,k) != 0) {
                tripletList.push_back(T(i, k, matSpharaMultFirst(i,k)));
            }
        }
    }

    SparseMatrix<double> matSparseSpharaMultFirst = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0) {
        matSparseSpharaMultFirst.setFromTriplets(tripletList.begin(), tripletList.end());
    }

    //Second operator
    tripletList.clear();
    tripletList.reserve(matSpharaMultSecond.rows()*matSpharaMultSecond.cols());

    for(i = 0; i < matSpharaMultSecond.rows(); ++i) {
        for(k = 0; k < matSpharaMultSecond.cols(); ++k) {
            if(matSpharaMultSecond(i,k) != 0) {
                tripletList.push_back(T(i, k, matSpharaMultSecond(i,k)));
            }
        }
    }

    SparseMatrix<double>matSparseSpharaMultSecond = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0) {
        matSparseSpharaMultSecond.setFromTriplets(tripletList.begin(), tripletList.end());
    }

    //Create full multiplication matrix
    m_matSparseSpharaMult = matSparseSpharaMultFirst * matSparseSpharaMultSecond;

    m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;

    m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo) {
        msleep(10);// Wait for fiff Info
    }

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

        //Do SSP's and compensators here
        if(m_bCompActivated) {
            if(m_bProjActivated) {
                //Comp + Proj
                t_mat = m_matSparseProjCompMult * t_mat;
            } else {
                //Comp
                t_mat = m_matSparseCompMult * t_mat;
            }
        } else {
            if(m_bProjActivated) {
                //Proj
                t_mat = m_matSparseProjMult * t_mat;
            } else {
                //None - Raw
            }
        }

        //Do temporal filtering here
        if(m_bFilterActivated) {
            t_mat = m_pRtFilter->filterChannelsConcurrently(t_mat, m_iMaxFilterLength, m_lFilterChannelList, m_filterData);
        }

//        qDebug()<<"t_mat dim:"<<t_mat.rows()<<"x"<<t_mat.cols();
//        qDebug()<<"m_lFilterChannelList.size():"<<m_lFilterChannelList.size();
//        qDebug()<<"m_filterData.size():"<<m_filterData.size();

        //Do SPHARA here
        if(m_bSpharaActive) {
            //Set bad channels to zero so they do not get smeared into
            for(int i = 0; i < m_pFiffInfo->bads.size(); ++i) {
                t_mat.row(m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(i))).setZero();
            }

            t_mat = m_matSparseSpharaMult * t_mat;
        }

//        //Common average
//        MatrixXd commonAvr = MatrixXd(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//        commonAvr.setZero();

//        int nEEGCh = 0;

//        for(int i = 0; i <m_pFiffInfo->chs.size(); ++i) {
//            if(m_pFiffInfo->chs.at(i).ch_name.contains("EEG") && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
//                nEEGCh++;
//            }
//        }

//        for(int i = 0; i <m_pFiffInfo->chs.size(); ++i) {
//            for(int j = 0; j < m_pFiffInfo->chs.size(); ++j) {
//                if(m_pFiffInfo->chs.at(j).ch_name.contains("EEG") && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(j).ch_name)) {
//                    commonAvr(i,j) = 1/nEEGCh;
//                }
//            }
//        }

//        UTILSLIB::IOUtils::write_eigen_matrix(commonAvr, "commonAvr.txt", "common vaergae matrix");

        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
        m_pNoiseReductionOutput->data()->setValue(t_mat);
    }
}
