//=============================================================================================================
/**
 * @file     hpi.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the Hpi class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpi.h"

#include "FormFiles/hpisetupwidget.h"

#include <disp/viewers/hpisettingsview.h>
#include <scMeas/realtimemultisamplearray.h>
#include <inverse/hpiFit/hpifit.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace HPIPLUGIN;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace Eigen;
using namespace RTPROCESSINGLIB;
using namespace INVERSELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Hpi::Hpi()
: m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr::create(40))
, m_bDoContinousHpi(false)
, m_bUseSSP(false)
, m_bUseComp(false)
, m_bDoSingleHpi(false)
{
}

//=============================================================================================================

Hpi::~Hpi()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> Hpi::clone() const
{
    QSharedPointer<Hpi> pHpiClone(new Hpi);
    return pHpiClone;
}

//=============================================================================================================

void Hpi::init()
{
    // Input
    m_pHpiInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "HpiIn", "Hpi input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &Hpi::update, Qt::DirectConnection);

    m_inputConnectors.append(m_pHpiInput);
}

//=============================================================================================================

void Hpi::unload()
{
}

//=============================================================================================================

bool Hpi::start()
{
    //Start thread as soon as we have received the first data block. See update().

    return true;
}

//=============================================================================================================

bool Hpi::stop()
{
    requestInterruption();
    wait(500);

    m_pFiffInfo = Q_NULLPTR;

    return true;
}

//=============================================================================================================

IPlugin::PluginType Hpi::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Hpi::getName() const
{
    return "HPI Fitting";
}

//=============================================================================================================

QWidget* Hpi::setupWidget()
{
    HpiSetupWidget* setupWidget = new HpiSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Hpi::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            initPluginControlWidgets();

            updateProjections();

            QThread::start();
        }

        // Check if data is present
        if(pRTMSA->getMultiSampleArray().size() > 0) {
            //If bad channels changed, recalcluate projectors
            //m_mutex.lock();
            if(m_iNumberBadChannels != m_pFiffInfo->bads.size()
               || m_matCompProjectors.rows() == 0
               || m_matCompProjectors.cols() == 0) {
                updateProjections();
                m_iNumberBadChannels = m_pFiffInfo->bads.size();
            }
            //m_mutex.unlock();

            if(m_bDoSingleHpi) {
                while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[0])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }

                m_bDoSingleHpi = false;
            }

            if(m_bDoContinousHpi) {
                for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                    // Please note that we do not need a copy here since this function will block until
                    // the buffer accepts new data again. Hence, the data is not deleted in the actual
                    // Mesaurement function after it emitted the notify signal.
                    while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                        //Do nothing until the circular buffer is ready to accept new data again
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void Hpi::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projects Settings
        HpiSettingsView* pHpiSettingsView = new HpiSettingsView(QString("MNESCAN/%1/").arg(this->getName()));
        pHpiSettingsView->setObjectName("widget_");

        connect(pHpiSettingsView, &HpiSettingsView::digitizersChanged,
                this, &Hpi::onDigitizersChanged);
        connect(pHpiSettingsView, &HpiSettingsView::doSingleHpiFit,
                this, &Hpi::onDoSingleHpiFit);
        connect(pHpiSettingsView, &HpiSettingsView::coilFrequenciesChanged,
                this, &Hpi::onCoilFrequenciesChanged);
        connect(pHpiSettingsView, &HpiSettingsView::sspStatusChanged,
                this, &Hpi::onSspStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::compStatusChanged,
                this, &Hpi::onCompStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::contHpiStatusChanged,
                this, &Hpi::onContHpiStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::allowedMeanErrorDistChanged,
                this, &Hpi::onAllowedMeanErrorDistChanged);
        connect(this, &Hpi::errorsChanged,
                pHpiSettingsView, &HpiSettingsView::setErrorLabels, Qt::BlockingQueuedConnection);

        onSspStatusChanged(pHpiSettingsView->getSspStatusChanged());
        onCompStatusChanged(pHpiSettingsView->getCompStatusChanged());
        onAllowedMeanErrorDistChanged(pHpiSettingsView->getAllowedMeanErrorDistChanged());

        plControlWidgets.append(pHpiSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    }
}

//=============================================================================================================

void Hpi::updateProjections()
{
    if(m_pFiffInfo) {
        m_matProjectors = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
        Eigen::MatrixXd matComp = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

        if(m_bUseSSP) {
            // Use SSP + SGM + calibration
            //Do a copy here because we are going to change the activity flags of the SSP's
            FiffInfo infoTemp = *(m_pFiffInfo.data());

            //Turn on all SSP
            for(int i = 0; i < infoTemp.projs.size(); ++i) {
                infoTemp.projs[i].active = true;
            }

            //Create the projector for all SSP's on
            infoTemp.make_projector(m_matProjectors);
            //set columns of matrix to zero depending on bad channels indexes
            for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
                m_matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
            }
        }

        if(m_bUseComp) {
            // Setup Comps
            FiffCtfComp newComp;
            //Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
            if(m_pFiffInfo->make_compensator(0, 101, newComp)) {
                matComp = newComp.data->data;
            }
        }

        m_mutex.lock();
        m_matCompProjectors = m_matProjectors * matComp;
        m_mutex.unlock();
    }
}

//=============================================================================================================

void Hpi::onAllowedMeanErrorDistChanged(double dAllowedMeanErrorDist)
{
    m_dAllowedMeanErrorDist = dAllowedMeanErrorDist;
}

//=============================================================================================================

void Hpi::onDigitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers)
{    
    m_mutex.lock();
    if(m_pFiffInfo) {
        m_pFiffInfo->dig = lDigitzers;
    }
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onDoSingleHpiFit()
{
    if(m_vCoilFreqs.size() < 3) {
       QMessageBox msgBox;
       msgBox.setText("Please load a digitizer set with at least 3 HPI coils first.");
       msgBox.exec();
       return;
    }

    m_bDoSingleHpi = true;
}

//=============================================================================================================

void Hpi::onCoilFrequenciesChanged(const QVector<int>& vCoilFreqs)
{
    m_mutex.lock();
    m_vCoilFreqs = vCoilFreqs;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onSspStatusChanged(bool bChecked)
{
    m_bUseSSP = bChecked;

    updateProjections();
}

//=============================================================================================================

void Hpi::onCompStatusChanged(bool bChecked)
{
    m_bUseComp = bChecked;
    updateProjections();
}

//=============================================================================================================

void Hpi::onContHpiStatusChanged(bool bChecked)
{
    m_bDoContinousHpi = bChecked;
}

//=============================================================================================================

void Hpi::run()
{
    HpiFitResult fitResult;
    MatrixXd matData;
    double dErrorMax = 0.0;

    while(!isInterruptionRequested()) {
        //pop matrix
        if(m_pCircularBuffer->pop(matData)) {
            // Perform HPI fit
            //Perform actual fitting
            fitResult.devHeadTrans.from = 1;
            fitResult.devHeadTrans.to = 4;

            m_mutex.lock();
            HPIFit::fitHPI(matData,
                           m_matCompProjectors,
                           fitResult.devHeadTrans,
                           m_vCoilFreqs,
                           fitResult.errorDistances,
                           fitResult.GoF,
                           fitResult.fittedCoils,
                           m_pFiffInfo);
            m_mutex.unlock();

            //Check if the error meets distance requirement
            if(fitResult.errorDistances.size() > 0) {
                double dMeanErrorDist = 0;
                dMeanErrorDist = std::accumulate(fitResult.errorDistances.begin(), fitResult.errorDistances.end(), .0) / fitResult.errorDistances.size();

                emit errorsChanged(fitResult.errorDistances, dMeanErrorDist);

                m_mutex.lock();
                dErrorMax = m_dAllowedMeanErrorDist;
                m_mutex.unlock();
                if(dMeanErrorDist < dErrorMax) {
                    //Dispatch
                }
            }
        }
    }
}
