//=============================================================================================================
/**
 * @file     noisereduction.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the NoiseReduction class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction.h"

#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/spharasettingsview.h>

#include <rtprocessing/filter.h>
#include <rtprocessing/sphara.h>

#include <utils/ioutils.h>

#include <scMeas/realtimemultisamplearray.h>

#include "FormFiles/noisereductionsetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NOISEREDUCTIONPLUGIN;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace UTILSLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReduction::NoiseReduction()
: m_bCompActivated(false)
, m_bSpharaActive(false)
, m_bProjActivated(false)
, m_bFilterActivated(false)
, m_iMaxFilterLength(1)
, m_iMaxFilterTapSize(-1)
, m_sCurrentSystem("VectorView")
, m_pCircularBuffer(QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>::create(40))
, m_pNoiseReductionInput(Q_NULLPTR)
, m_pNoiseReductionOutput(Q_NULLPTR)
{
    if(m_sCurrentSystem == "BabyMEG") {
        m_iNBaseFctsFirst = 270;
        m_iNBaseFctsSecond = 105;
    } else if(m_sCurrentSystem == "VectorView") {
        m_iNBaseFctsFirst = 102;
        m_iNBaseFctsSecond = 102;
    } else {
        m_iNBaseFctsFirst = 0;
        m_iNBaseFctsSecond = 0;
        qDebug() << "[NoiseReduction::NoiseReduction] Current system type not recognized.";
    }
}

//=============================================================================================================

NoiseReduction::~NoiseReduction()
{
    if(this->isRunning()) {
        NoiseReduction::stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> NoiseReduction::clone() const
{
    QSharedPointer<NoiseReduction> pNoiseReductionClone(new NoiseReduction);
    return pNoiseReductionClone;
}

//=============================================================================================================

void NoiseReduction::init()
{
    // Input
    m_pNoiseReductionInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NoiseReductionIn", "NoiseReduction input data");
    connect(m_pNoiseReductionInput.data(), &PluginInputConnector::notify,
            this, &NoiseReduction::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNoiseReductionInput);

    // Output
    m_pNoiseReductionOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "NoiseReductionOut", "NoiseReduction output data");
    m_pNoiseReductionOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pNoiseReductionOutput);
}

//=============================================================================================================

void NoiseReduction::unload()
{
}

//=============================================================================================================

bool NoiseReduction::start()
{
    //Start thread as soon as we have received the first data block. See update().

    return true;
}

//=============================================================================================================

bool NoiseReduction::stop()
{
    requestInterruption();
    wait(500);

    m_iMaxFilterTapSize = -1;

    m_pNoiseReductionOutput->measurementData()->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType NoiseReduction::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString NoiseReduction::getName() const
{
    return "Filter";
}

//=============================================================================================================

QWidget* NoiseReduction::setupWidget()
{
    NoiseReductionSetupWidget* setupWidget = new NoiseReductionSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void NoiseReduction::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

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

            //Init output
            m_pNoiseReductionOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
            m_pNoiseReductionOutput->measurementData()->setMultiArraySize(1);
        }

        // Check if data is present
        if(pRTMSA->getMultiSampleArray().size() > 0) {
            //Init widgets
            if(m_iMaxFilterTapSize == -1) {
                m_iMaxFilterTapSize = pRTMSA->getMultiSampleArray().first().cols();
                initPluginControlWidgets();
                QThread::start();
            }

            for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                // Please note that we do not need a copy here since this function will block until
                // the buffer accepts new data again. Hence, the data is not deleted in the actual
                // Measurement function after it emitted the notify signal.
                while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }
        }
    }
}

//=============================================================================================================

void NoiseReduction::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projectors
        ProjectorsView* pProjectorsView = new ProjectorsView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pProjectorsView, &ProjectorsView::setGuiMode);
        pProjectorsView->setObjectName("group_tab_Settings_SSP");
        plControlWidgets.append(pProjectorsView);

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                this, &NoiseReduction::updateProjection);

        pProjectorsView->setProjectors(m_pFiffInfo->projs);

        // Compensators
        CompensatorView* pCompensatorView = new CompensatorView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pCompensatorView, &CompensatorView::setGuiMode);
        pCompensatorView->setObjectName("group_tab_Settings_Comp");
        plControlWidgets.append(pCompensatorView);

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                this, &NoiseReduction::updateCompensator);

        pCompensatorView->setCompensators(m_pFiffInfo->comps);

        // Filter
        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pFilterSettingsView, &FilterSettingsView::setGuiMode);
        pFilterSettingsView->setObjectName("group_tab_Settings_Filter");
        plControlWidgets.append(pFilterSettingsView);

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
                this, &NoiseReduction::setFilterChannelType);

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
                this, &NoiseReduction::setFilter);

        connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
                this, &NoiseReduction::setFilterActive);

        pFilterSettingsView->setSamplingRate(m_pFiffInfo->sfreq);
        pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(m_iMaxFilterTapSize);

        this->setFilterActive(pFilterSettingsView->getFilterActive());
        this->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());

        // SPHARA settings
        SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView(QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pSpharaSettingsView, &SpharaSettingsView::setGuiMode);
        pSpharaSettingsView->setObjectName("group_tab_Settings_SPHARA");
        plControlWidgets.append(pSpharaSettingsView);

        connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
                this, &NoiseReduction::setSpharaActive);

        connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
                this, &NoiseReduction::setSpharaOptions);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    }
}

//=============================================================================================================

void NoiseReduction::setSpharaActive(bool state)
{
    m_mutex.lock();
    m_bSpharaActive = state;
    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setSpharaOptions(const QString& sSytemType,
                                      int nBaseFctsFirst,
                                      int nBaseFctsSecond)
{
    m_mutex.lock();
    m_iNBaseFctsFirst = nBaseFctsFirst;
    m_iNBaseFctsSecond = nBaseFctsSecond;
    m_sCurrentSystem = sSytemType;
    m_mutex.unlock();

    createSpharaOperator();
}

//=============================================================================================================

void NoiseReduction::run()
{
    // Read and create SPHARA operator for the first time
    initSphara();
    createSpharaOperator();

    // Init
    MatrixXd matData;
    QScopedPointer<RTPROCESSINGLIB::FilterOverlapAdd> pRtFilter(new RTPROCESSINGLIB::FilterOverlapAdd());

    while(!isInterruptionRequested()) {
        // Get the current data
        if(m_pCircularBuffer->pop(matData)) {
            m_mutex.lock();
            //Do SSP's and compensators here
            if(m_bCompActivated) {
                if(m_bProjActivated) {
                    //Comp + Proj
                    matData = m_matSparseProjCompMult * matData;
                } else {
                    //Comp
                    matData = m_matSparseCompMult * matData;
                }
            } else {
                if(m_bProjActivated) {
                    //Proj
                    matData = m_matSparseProjMult * matData;
                } else {
                    //None - Raw
                }
            }

            //Do temporal filtering here
            if(m_bFilterActivated) {
                matData = pRtFilter->calculate(matData,
                                               m_filterKernel,
                                               m_lFilterChannelList);
            }

            //Do SPHARA here
            if(m_bSpharaActive) {
                //Set bad channels to zero so they do not get smeared into
                for(int i = 0; i < m_pFiffInfo->bads.size(); ++i) {
                    matData.row(m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(i))).setZero();
                }

                matData = m_matSparseSpharaMult * matData;
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

            //Send the data to the connected plugins and the display
            if(!isInterruptionRequested()) {
                m_pNoiseReductionOutput->measurementData()->setValue(matData);
            }
        }
    }
}

//=============================================================================================================

void NoiseReduction::updateProjection(const QList<FIFFLIB::FiffProj>& projs)
{
    //  Update the SSP projector
    if(m_pFiffInfo) {
        m_mutex.lock();
        //If a minimum of one projector is active set m_bProjActivated to true so that this model applies the ssp to the incoming data
        m_bProjActivated = false;
        for(qint32 i = 0; i < projs.size(); ++i) {
            if(projs[i].active) {
                m_bProjActivated = true;
                break;
            }
        }

        MatrixXd matProj;
        FiffProj::make_projector(projs, m_pFiffInfo->ch_names, matProj, m_pFiffInfo->bads);

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_pFiffInfo->bads.size(); ++j) {
            int index = m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(j));
            if(index >= 0 && index<m_pFiffInfo->ch_names.size()) {
                matProj.col(index).setZero();
            }
        }

        // Make proj sparse
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

//=============================================================================================================

void NoiseReduction::updateCompensator(int to)
{
    // Update the compensator
    if(m_pFiffInfo) {
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

        //this->m_pFiffInfo->set_current_comp(to);
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

//=============================================================================================================

void NoiseReduction::setFilterChannelType(QString sType)
{
    m_sFilterChannelType = sType;

    m_mutex.lock();
    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_lFilterChannelList.resize(0);

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {

            if(m_sFilterChannelType == "All") {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            }
        }
    }
    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setFilter(const FilterKernel& filterData)
{
    m_mutex.lock();
    m_filterKernel = filterData;

    m_iMaxFilterLength = 1;
    if(m_iMaxFilterLength < m_filterKernel.getFilterOrder()) {
        m_iMaxFilterLength = m_filterKernel.getFilterOrder();
    }
    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setFilterActive(bool state)
{
    m_bFilterActivated = state;
}

//=============================================================================================================

void NoiseReduction::initSphara()
{
    //Load SPHARA matrix
    IOUtils::read_eigen_matrix(m_matSpharaVVGradLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaVVMagLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaEEGLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Current_SPHARA_EEG.txt"));

    //Generate indices used to create the SPHARA operators for VectorView
    m_vecIndicesFirstVV.resize(0);
    m_vecIndicesSecondVV.resize(0);

    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find gardiometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3012) {
            m_vecIndicesFirstVV.conservativeResize(m_vecIndicesFirstVV.rows()+1);
            m_vecIndicesFirstVV(m_vecIndicesFirstVV.rows()-1) = r;
        }

        //Find magnetometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3024) {
            m_vecIndicesSecondVV.conservativeResize(m_vecIndicesSecondVV.rows()+1);
            m_vecIndicesSecondVV(m_vecIndicesSecondVV.rows()-1) = r;
        }
    }

    //Generate indices used to create the SPHARA operators for babyMEG
    m_vecIndicesFirstBabyMEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find inner layer
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

//=============================================================================================================

void NoiseReduction::createSpharaOperator()
{
    qDebug()<<"NoiseReduction::createSpharaOperator - Creating SPHARA oerpator for"<<m_sCurrentSystem;

    m_mutex.lock();

    MatrixXd matSpharaMultFirst = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    MatrixXd matSpharaMultSecond = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    if(m_sCurrentSystem == "VectorView") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVGradLoaded, m_vecIndicesFirstVV, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 1); //GRADIOMETERS
        matSpharaMultSecond = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVMagLoaded, m_vecIndicesSecondVV, m_pFiffInfo->nchan, m_iNBaseFctsSecond, 0); //Magnetometers
    }

    if(m_sCurrentSystem == "BabyMEG") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaBabyMEGInnerLoaded, m_vecIndicesFirstBabyMEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    if(m_sCurrentSystem == "EEG") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaEEGLoaded, m_vecIndicesFirstEEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    //Write final operator matrices to file
//    IOUtils::write_eigen_matrix(matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "../resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultFirst.txt"));
//    IOUtils::write_eigen_matrix(matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "../resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultSecond.txt"));

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

//=============================================================================================================

QString NoiseReduction::getBuildInfo()
{
    return QString(NOISEREDUCTIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(NOISEREDUCTIONPLUGIN::buildHash());
}
