//=============================================================================================================
/**
 * @file     neurofeedback.cpp
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Definition of the Neurofeedback class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedback.h"
#include "FormFiles/neurofeedbacksetupwidget.h"

#include <disp/viewers/neurofeedbackcsettingsview.h>
#include <disp/viewers/neurofeedbackfsettingsview.h>
#include <disp/viewers/neurofeedbackbsettingsview.h>
#include <disp/viewers/neurofeedbacksettingsview.h>

#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeneurofeedbackresult.h>

#include <scDisp/realtimeneurofeedbackwidget.h>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROFEEDBACKPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neurofeedback::Neurofeedback()
: m_pNeurofeedbackInput(NULL)
, m_pNeurofeedbackOutput(NULL)
, m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_double>(new CircularBuffer_Matrix_double(10)))
{
}

//=============================================================================================================

Neurofeedback::~Neurofeedback()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

void Neurofeedback::init()
{
    // Input
    m_pNeurofeedbackInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NeurofeedbackIn", "Neurofeedback input data");
    connect(m_pNeurofeedbackInput.data(), &PluginInputConnector::notify, this, &Neurofeedback::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNeurofeedbackInput);

    // Output
    m_pNeurofeedbackOutput = PluginOutputData<RealTimeNeurofeedbackResult>::create(this, "NeurofeedbackOut", "Neurofeedback Output Data");
    m_pNeurofeedbackOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pNeurofeedbackOutput);

}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Neurofeedback::clone() const
{
    QSharedPointer<Neurofeedback> pNeurofeedbackClone(new Neurofeedback);
    return pNeurofeedbackClone;
}

//=============================================================================================================

void Neurofeedback::unload()
{
}

//=============================================================================================================

bool Neurofeedback::start()
{
    QThread::start();

    return true;
}

//=============================================================================================================

bool Neurofeedback::stop()
{
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    m_pNeurofeedbackOutput->measurementData().clear();
    m_pCircularBuffer->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Neurofeedback::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Neurofeedback::getName() const
{
    return "Neurofeedback";
}

//=============================================================================================================

QWidget* Neurofeedback::setupWidget()
{
    NeurofeedbackSetupWidget* setupWidget = new NeurofeedbackSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Neurofeedback::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    qDebug()<<"Neurofeedback::update";
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pNeurofeedbackOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
            m_pNeurofeedbackOutput->measurementData()->setMultiArraySize(1);

        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        emit updateMin(m_pNeurofeedbackOutput->measurementData()->getFMin());
        emit updateMax(m_pNeurofeedbackOutput->measurementData()->getFMax());

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            // Please note that we do not need a copy here since this function will block until
            // the buffer accepts new data again. Hence, the data is not deleted in the actual
            // Mesaurement function after it emitted the notify signal.
            while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                //Do nothing until the circular buffer is ready to accept new data again
            }

        }
    }
}



//=============================================================================================================

void Neurofeedback::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plNControlWidgets;

        bool bMaxAutoScale = m_pNeurofeedbackOutput->measurementData()->getMaxAutoScale();
        bool bMinAutoScale = m_pNeurofeedbackOutput->measurementData()->getMinAutoScale();
        QString sOutput = m_pNeurofeedbackOutput->measurementData()->getNeuroOutput();
        int iMax = 0;
        int iMin = 0;

        if(sOutput == "Frequency"){
            iMax = m_pNeurofeedbackOutput->measurementData()->getFMax();
            iMin = m_pNeurofeedbackOutput->measurementData()->getFMin();
        }
        else if(sOutput == "Balloon"){
            iMax = m_pNeurofeedbackOutput->measurementData()->getBMax();
            iMin = m_pNeurofeedbackOutput->measurementData()->getBMin();
        }

        NeurofeedbackSettingsView* pNeurofeedbackSettingsView = new NeurofeedbackSettingsView(QString("MNESCAN/%1/").arg(this->getName()), iMin, iMax,
                                                                                              bMinAutoScale, bMaxAutoScale, 0);
        pNeurofeedbackSettingsView->setObjectName("group_tab_Settings_Neurofeedback");
        plNControlWidgets.append(pNeurofeedbackSettingsView);

        if(sOutput == "Frequency"){
            connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MaxChanged, this, &Neurofeedback::changeFMax);
            connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MinChanged, this, &Neurofeedback::changeFMin);
            connect(this, &Neurofeedback::updateMax, pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::changeMax);
            connect(this, &Neurofeedback::updateMin, pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::changeMin);
        }
        else if(sOutput == "Balloon"){
            connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MaxChanged, this, &Neurofeedback::changeBMax);
            connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MinChanged, this, &Neurofeedback::changeBMin);
        }

        connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MaxAutoScaleChanged, this, &Neurofeedback::changeMaxAutoScale);
        connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::MinAutoScaleChanged, this, &Neurofeedback::changeMinAutoScale);
        connect(pNeurofeedbackSettingsView, &NeurofeedbackSettingsView::ResetAutoScaleChanged, this, &Neurofeedback::changeResetAutoScale);




        emit pluginControlWidgetsChanged(plNControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}



//=============================================================================================================

void Neurofeedback::run()

{
        MatrixXd matData;

        // Wait for Fiff Info
        while(!m_pFiffInfo) {
            msleep(10);
        }
        while(!isInterruptionRequested()) {
            // Get the current data
            if(m_pCircularBuffer->pop(matData)) {
                //ToDo: Implement your algorithm here

                //Send the data to the connected plugins and the online display
                //Unocmment this if you also uncommented the m_pOutput in the constructor above
                if(!isInterruptionRequested()) {
                    m_pNeurofeedbackOutput->measurementData()->setValue(matData);
                }
            }
        }
}


//==============================================================================================================
void Neurofeedback::changeOutput(int value){
    if(value == 0){
        m_pNeurofeedbackOutput->measurementData()->setNeuroOutput("Classifier");
    }
    else if(value == 1){
        m_pNeurofeedbackOutput->measurementData()->setNeuroOutput("Frequency");
    }
    else{
        m_pNeurofeedbackOutput->measurementData()->setNeuroOutput("Balloon");
    }
}

//==============================================================================================================
void Neurofeedback::changeSliders(int value){
    m_pNeurofeedbackOutput->measurementData()->setSlider(value);
}

//==============================================================================================================
void Neurofeedback::changeballCh(bool ballCh){
    m_pNeurofeedbackOutput->measurementData()->setallCh(ballCh);
}

//==============================================================================================================
void Neurofeedback::changeNumbofClass(int value){
    m_pNeurofeedbackOutput->measurementData()->setNumbofClass(value);
}

//==============================================================================================================
void Neurofeedback::changeClass0(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassL(text, 0);
}
//==============================================================================================================
void Neurofeedback::changeClass1(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassL(text, 1);
}

//===============================================================================================================
void Neurofeedback::changeClass2(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassL(text, 2);
}

//===============================================================================================================
void Neurofeedback::changeClass3(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassL(text, 3);
}

//===============================================================================================================
void Neurofeedback::changeClass4(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassL(text, 4);
}

//===============================================================================================================
void Neurofeedback::changeGBC0(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassCol(text, 0);
}

//===============================================================================================================
void Neurofeedback::changeGBC1(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassCol(text, 1);
}

//===============================================================================================================
void Neurofeedback::changeGBC2(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassCol(text, 2);
}

//===============================================================================================================
void Neurofeedback::changeGBC3(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassCol(text, 3);
}

//===============================================================================================================
void Neurofeedback::changeGBC4(QString text){
    m_pNeurofeedbackOutput->measurementData()->setClassCol(text, 4);
}

//===============================================================================================================
void Neurofeedback::changeiClass0(int value){
    m_pNeurofeedbackOutput->measurementData()->setClassI(value, 0);
}

//===============================================================================================================
void Neurofeedback::changeiClass1(int value){
    m_pNeurofeedbackOutput->measurementData()->setClassI(value, 1);
}

//===============================================================================================================
void Neurofeedback::changeiClass2(int value){
    m_pNeurofeedbackOutput->measurementData()->setClassI(value, 2);
}

//===============================================================================================================
void Neurofeedback::changeiClass3(int value){
    m_pNeurofeedbackOutput->measurementData()->setClassI(value, 3);
}

//===============================================================================================================
void Neurofeedback::changeiClass4(int value){
    m_pNeurofeedbackOutput->measurementData()->setClassI(value, 4);
}

//===============================================================================================================
void Neurofeedback::changeImgClass0(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setClassImg(image, 0);
}

//===============================================================================================================
void Neurofeedback::changeImgClass1(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setClassImg(image, 1);
}

//===============================================================================================================
void Neurofeedback::changeImgClass2(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setClassImg(image, 2);
}

//===============================================================================================================
void Neurofeedback::changeImgClass3(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setClassImg(image, 3);
}

//===============================================================================================================
void Neurofeedback::changeImgClass4(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setClassImg(image, 4);
}

//=============================================================================================================
void Neurofeedback::changeImgBackground(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setImgBackground(image);
}

//=============================================================================================================

void Neurofeedback::changeImgObject(QPixmap image){
    m_pNeurofeedbackOutput->measurementData()->setImgObject(image);
}

//=============================================================================================================

void Neurofeedback::changeBMax(int value){
    m_pNeurofeedbackOutput->measurementData()->setBMax(value);
}

//=============================================================================================================

void Neurofeedback::changeBMin(int value){
    m_pNeurofeedbackOutput->measurementData()->setBMin(value);
}

//=============================================================================================================

void Neurofeedback::changeFMax(int value){
    m_pNeurofeedbackOutput->measurementData()->setFMax(value);
}

//=============================================================================================================

void Neurofeedback::changeFMin(int value){
    m_pNeurofeedbackOutput->measurementData()->setFMin(value);
}

//=============================================================================================================

void Neurofeedback::changeMaxAutoScale(bool bMaxAutoScale){
    m_pNeurofeedbackOutput->measurementData()->setMaxAutoScale(bMaxAutoScale);
}

//=============================================================================================================

void Neurofeedback::changeMinAutoScale(bool bMinAutoScale){
    m_pNeurofeedbackOutput->measurementData()->setMinAutoScale(bMinAutoScale);
}

//=============================================================================================================

void Neurofeedback::changeResetAutoScale(bool bResetAutoScale){
    m_pNeurofeedbackOutput->measurementData()->setResetAutoScale(bResetAutoScale);
}

//=============================================================================================================

void Neurofeedback::changeChannel(int value){
    m_pNeurofeedbackOutput->measurementData()->setChannel(value);
}

//=============================================================================================================

QString Neurofeedback::getBuildInfo()
{
    return QString(NEUROFEEDBACKPLUGIN::buildDateTime()) + QString(" - ")  + QString(NEUROFEEDBACKPLUGIN::buildHash());
}
