//=============================================================================================================
/**
 * @file     timefrequency.cpp
 * @author   Juan Garcia-Prieto <jgarciaprieto@nmr.mgh.harvard.edu>;
 * @since    0.1.0
 * @date     March, 2023
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
 * @brief    Definition of the TimeFrequency class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency.h"

// #include <disp/viewers/scalingview.h>
// #include <disp/viewers/projectorsview.h>
// #include <disp/viewers/filtersettingsview.h>
// #include <disp/viewers/filterdesignview.h>
// #include <disp/viewers/compensatorview.h>
// #include <disp/viewers/spharasettingsview.h>
//
// #include <rtprocessing/filter.h>
// #include <rtprocessing/sphara.h>
//
#include <utils/ioutils.h>

#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

// using namespace SCMEASLIB;
// using namespace UTILSLIB;
// using namespace DISPLIB;
// using namespace RTPROCESSINGLIB;
// using namespace FIFFLIB;
// using namespace SCSHAREDLIB;
// using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
namespace TIMEFREQUENCYPLUGIN {


TimeFrequency::TimeFrequency()
: m_pTimeFrequencyIntput(Q_NULLPTR)
// : m_bCompActivated(false)
// , m_bSpharaActive(false)
// , m_bProjActivated(false)
// , m_bFilterActivated(false)
// , m_iMaxFilterLength(1)
// , m_iMaxFilterTapSize(-1)
// , m_sCurrentSystem("VectorView")
// , m_pCircularBuffer(QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>::create(40))
{
    qDebug() << "[TimeFrequency::TimeFrequency] Creating Plugin Object.";
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TimeFrequency::clone() const
{
    QSharedPointer<TimeFrequency> pTimeFrequencyClone(new TimeFrequency);
    return pTimeFrequencyClone;
}

//=============================================================================================================

void TimeFrequency::init()
{
    // Input
    m_pTimeFrequencyInput = PluginInputData<RealTimeMultiSampleArray>::
        create(this, "TimeFrequencyIn", "TimeFrequency input data");
    m_inputConnectors.append(m_pTimeFrequencyInput);
    m_pTimeFrequencyInput->measurementData()->setName(this->getName());//Provide name to auto store widget settings

    // Output
    // m_pTimeFrequencyOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "TimeFrequencyOut", "TimeFrequency output data");
    // m_outputConnectors.append(m_pTimeFrequencyOutput);
}

//=============================================================================================================

void TimeFrequency::unload()
{
}

//=============================================================================================================

bool TimeFrequency::start()
{
    //Start thread as soon as we have received the first data block. See update().

    return true;
}

//=============================================================================================================

bool TimeFrequency::stop()
{
    requestInterruption();
   
    wait(500);

    // m_pTimeFrequencyOutput->measurementData()->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType TimeFrequency::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString TimeFrequency::getName() const
{
    return "TimeFrequency";
}

//=============================================================================================================

QWidget* TimeFrequency::setupWidget()
{
    // TimeFrequencySetupWidget* setupWidget = new TimeFrequencySetupWidget(this);  
    // // widget is later distroyed by CentralWidget - so it has to be created everytime new
    QLabel* setupWidget("Time Frequency Baby");
    return setupWidget;
}

//=============================================================================================================

// void TimeFrequency::update(SCMEASLIB::Measurement::SPtr pMeasurement)
// {
//     if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
//         //Check if the fiff info was inititalized
//         if(!m_pFiffInfo) {
//             m_pFiffInfo = pRTMSA->info();
//
//             //Init the multiplication matrices
//             m_matSparseProjMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//             m_matSparseCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//             m_matSparseSpharaMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//             m_matSparseProjCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//             m_matSparseFull = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
//
//             m_matSparseProjMult.setIdentity();
//             m_matSparseCompMult.setIdentity();
//             m_matSparseSpharaMult.setIdentity();
//             m_matSparseProjCompMult.setIdentity();
//             m_matSparseFull.setIdentity();
//
//             //Init output
//             m_pTimeFrequencyOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
//             m_pTimeFrequencyOutput->measurementData()->setMultiArraySize(1);
//         }
//
//         // Check if data is present
//         if(pRTMSA->getMultiSampleArray().size() > 0) {
//             //Init widgets
//             if(m_iMaxFilterTapSize == -1) {
//                 m_iMaxFilterTapSize = pRTMSA->getMultiSampleArray().first().cols();
//                 initPluginControlWidgets();
//                 QThread::start();
//             }
//
//             for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
//                 // Please note that we do not need a copy here since this function will block until
//                 // the buffer accepts new data again. Hence, the data is not deleted in the actual
//                 // Measurement function after it emitted the notify signal.
//                 while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
//                     //Do nothing until the circular buffer is ready to accept new data again
//                 }
//             }
//         }
//     }
// }
//
// //=============================================================================================================
//
// void TimeFrequency::initPluginControlWidgets()
// {
//     if(m_pFiffInfo) {
//         QList<QWidget*> plControlWidgets;
//
//         // Projectors
//         ProjectorsView* pProjectorsView = new ProjectorsView(QString("MNESCAN/%1/").arg(this->getName()));
//         connect(this, &TimeFrequency::guiModeChanged,
//                 pProjectorsView, &ProjectorsView::setGuiMode);
//         pProjectorsView->setObjectName("group_tab_Settings_SSP");
//         plControlWidgets.append(pProjectorsView);
//
//         connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
//                 this, &TimeFrequency::updateProjection);
//
//         pProjectorsView->setProjectors(m_pFiffInfo->projs);
//
//         // Compensators
//         CompensatorView* pCompensatorView = new CompensatorView(QString("MNESCAN/%1/").arg(this->getName()));
//         connect(this, &TimeFrequency::guiModeChanged,
//                 pCompensatorView, &CompensatorView::setGuiMode);
//         pCompensatorView->setObjectName("group_tab_Settings_Comp");
//         plControlWidgets.append(pCompensatorView);
//
//         connect(pCompensatorView, &CompensatorView::compSelectionChanged,
//                 this, &TimeFrequency::updateCompensator);
//
//         pCompensatorView->setCompensators(m_pFiffInfo->comps);
//
//         // Filter
//         FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("MNESCAN/%1/").arg(this->getName()));
//         connect(this, &TimeFrequency::guiModeChanged,
//                 pFilterSettingsView, &FilterSettingsView::setGuiMode);
//         pFilterSettingsView->setObjectName("group_tab_Settings_Filter");
//         plControlWidgets.append(pFilterSettingsView);
//
//         connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
//                 this, &TimeFrequency::setFilterChannelType);
//
//         connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
//                 this, &TimeFrequency::setFilter);
//
//         connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
//                 this, &TimeFrequency::setFilterActive);
//
//         pFilterSettingsView->setSamplingRate(m_pFiffInfo->sfreq);
//         pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(m_iMaxFilterTapSize);
//
//         this->setFilterActive(pFilterSettingsView->getFilterActive());
//         this->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());
//
//         // SPHARA settings
//         SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView(QString("MNESCAN/%1").arg(this->getName()));
//         connect(this, &TimeFrequency::guiModeChanged,
//                 pSpharaSettingsView, &SpharaSettingsView::setGuiMode);
//         pSpharaSettingsView->setObjectName("group_tab_Settings_SPHARA");
//         plControlWidgets.append(pSpharaSettingsView);
//
//         connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
//                 this, &TimeFrequency::setSpharaActive);
//
//         connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
//                 this, &TimeFrequency::setSpharaOptions);
//
//         emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
//     }
// }
//
// //=============================================================================================================
//
// void TimeFrequency::setSpharaActive(bool state)
// {
//     m_mutex.lock();
//     m_bSpharaActive = state;
//     m_mutex.unlock();
// }
//
// //=============================================================================================================
//
// void TimeFrequency::setSpharaOptions(const QString& sSytemType,
//                                       int nBaseFctsFirst,
//                                       int nBaseFctsSecond)
// {
//     m_mutex.lock();
//     m_iNBaseFctsFirst = nBaseFctsFirst;
//     m_iNBaseFctsSecond = nBaseFctsSecond;
//     m_sCurrentSystem = sSytemType;
//     m_mutex.unlock();
//
//     createSpharaOperator();
// }
//
//=============================================================================================================

void TimeFrequency::run()
{
    // Read and create SPHARA operator for the first time
    // initSphara();
    // createSpharaOperator();

    // Init
    MatrixXd matData;
    // matData.
    // QScopedPointer<RTPROCESSINGLIB::FilterOverlapAdd> pRtFilter(new RTPROCESSINGLIB::FilterOverlapAdd());

    while (true)
    {
        // Get the current data
        msleep(500);
        // Send the data to the connected plugins and the display
        if(!isInterruptionRequested()) {
            m_pTimeFrequencyInput->measurementData()->setValue(matData);
        }
    }
}

//=============================================================================================================

QString TimeFrequency::getBuildInfo()
{
    return QString(buildDateTime()) + QString(" - ")  + QString(buildHash()); 
}

}  // namespace TIMEFREQUENCYPLUGIN

