//=============================================================================================================
/**
* @file     triggercontrol.cpp
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>
*           Luise Lang <luise.lang@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze, Luise Lang and Christoph Dinh. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the TriggerControlclass.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggercontrol.h"
#include "FormFiles/triggercontrolsetupwidget.h"

#include "serialport.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USER DEFINES
//=============================================================================================================

//#define TIMEMEAS // Zeitmessung;
//#define BUFFERX1 // X1 determination
#define TIMEMUC // Zeitmessung MUC


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TriggerControlPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerControl::TriggerControl()
: m_pTriggerOutput(NULL)
, m_iBaud(115000)
, m_pSerialPort(new SerialPort) // initialize a new serial port
, m_iNumChs(0)
, m_bIsRunning(false)
, m_pDataSingleChannel(new dBuffer(1024))
{
    connect(this, &TriggerControl::sendByte, this, &TriggerControl::sendByteTo);
}


//*************************************************************************************************************

TriggerControl::~TriggerControl()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> TriggerControl::clone() const
{
    QSharedPointer<TriggerControl> pTriggerControlClone(new TriggerControl);
    return pTriggerControlClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void TriggerControl::init()
{
    // Beginn Zeitmessung
    // Input
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "TriggerControlInI", "TriggerControl input data I");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &TriggerControl::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);
    // Ende Zeitmessung*/



    m_pRTSAInput = PluginInputData<NewRealTimeSampleArray>::create(this, "TriggerControlInII", "TriggerControl input data II");
    connect(m_pRTSAInput.data(), &PluginInputConnector::notify, this, &TriggerControl::updateSingleChannel, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSAInput);


    // Output
    m_pTriggerOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "DummyOut", "Dummy output data");
    m_outputConnectors.append(m_pTriggerOutput);

    m_pTriggerOutput->data()->setName("Dummy Output");
    m_pTriggerOutput->data()->setUnit("");
    m_pTriggerOutput->data()->setMinValue(-200);
    m_pTriggerOutput->data()->setMaxValue(360);
    m_pTriggerOutput->data()->setSamplingRate(256.0/1.0);

}


//*************************************************************************************************************

bool TriggerControl::start()
{
    m_bIsRunning = true;
    QThread::start();

//    // ////////////////////////////

//    if(!m_pSerialPort->open())   // open Serial Port
//        std::cout << "Not able to open port - test" << std::endl;

//    // ////////////////////////////

#ifdef TIMEMUC

    if(!m_pSerialPort->open())   // open Serial Port
        std::cout << "Not able to open port - test" << std::endl;

#endif



#ifdef TIMEMEAS



 // Beginn Zeitmessung




    if(m_pSerialPort->open())   // open Serial Port
    {
        m_qTime.start();
        QByteArray t_data;
        m_pSerialPort->m_digchannel.replace(9,1); // select 1st digital channel
        //t_data.append(0x01);
        m_pSerialPort->encodedig();             // encode signal to m_data
//        t_data[0] = t_data[0]|0x01;
        m_pSerialPort->sendData(m_pSerialPort->m_data);

        m_qTime.start();
        //m_pSerialPort->sendData(t_data);
        std::cout << "data sent" << std::endl;
    }
    else
    {
        std::cout << "Sending not possible, please check settings" << std::endl;
    }
#endif


    return true;
}


//*************************************************************************************************************

bool TriggerControl::stop()
{
    m_pSerialPort->close();

    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();

    m_pDataSingleChannel->clear();



 //Beginn Zeitmessung
    double sum = 0;
    for(int i=0; i<m_vTimes.size(); i++)
        sum += m_vTimes[i];

    QFile file(qApp->applicationDirPath()+"/mne_x_plugins/resources/triggercontrol/t_meas.txt");
    for(int i=0; i<m_vTimes.size(); i++)
        sum += m_vTimes[i];

    std::cout << "Average time: " << sum/m_vTimes.size() << std::endl;
    std::cout << "Size m_vTimes: " << m_vTimes.size() << std::endl;

    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    for(int i=0; i<m_vTimes.size() ;i++)
        out << m_vTimes[i] << endl;

    // optional, as QFile destructor will already do it:
    file.close();

    m_pData.clear();
    m_vTimes.clear();
// Ende Zeitmessung


    return true;
}


//*************************************************************************************************************

IPlugin::PluginType TriggerControl::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString TriggerControl::getName() const
{
    return "Trigger Control";
}


//*************************************************************************************************************

QWidget* TriggerControl::setupWidget()
{
    TriggerControlSetupWidget* setupWidget = new TriggerControlSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

  //  QWidget* setupWidget = new QWidget;
    return setupWidget;
}


//*************************************************************************************************************

void TriggerControl::updateSingleChannel(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeSampleArray> pRTSA = pMeasurement.dynamicCast<NewRealTimeSampleArray>();
    if(pRTSA)
    {
        m_qMutex.lock();
        qint32 t_iSize = pRTSA->getArraySize();
        for(qint32 i = 0; i < t_iSize; ++i)
            m_pDataSingleChannel->push(pRTSA->getSampleArray()[i]);//Append sample wise
        m_qMutex.unlock();
    }
}


//*************************************************************************************************************

void TriggerControl::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    ///* Beginn Zeitmessung
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        m_qMutex.lock();
        m_iNumChs = pRTMSA->getNumChannels();
        qint32 t_iSize = pRTMSA->getMultiSampleArray().size();
        for(qint32 i = 0; i < t_iSize; ++i)
            m_pData.append(pRTMSA->getMultiSampleArray()[i]);//Append sample wise
        m_qMutex.unlock();
    }
    // ENDE Zeitmessung */
}




//*************************************************************************************************************


void TriggerControl::run()
{

#ifdef TIMEMUC
    connect(m_pSerialPort.data(), &SerialPort::byteReceived, this, &TriggerControl::byteReceived);


    m_isReceived = false;
    emit sendByte(1);
    m_qTime.start();

    while(m_bIsRunning)
    {
        if(m_isReceived)
        {
            m_vTimes.push_back(m_qTime.elapsed());
            m_isReceived = false;
            emit sendByte(1);
        }
    }
#endif



#ifdef TIMEMEAS

        //Beginn Zeitmessung - alpha zeitmessung

        m_pData.clear();


        bool t_bFound = false;
        double x_u = -0.01;
        double x_o = 0.01;

    //       int count = 0;


           while(m_bIsRunning)
           {
               m_qMutex.lock();

               if(t_bFound && m_pData.size() > 0)
               {
                  // m_pData.clear();
                   // ++count;

                   msleep(5000);
                   m_pData.clear();
                   t_bFound = false;

                 /*  if(count > 2)
                   {
                       t_bFound = false;
                       count = 0;

                       emit sendByte(1);
                       m_qTime.start();
                   }*/

               }

               if(m_pData.size() > 0)
               {
                   if((!t_bFound) && (m_pData.first()[m_iNumChs-2] > x_u) && (m_pData.first()[m_iNumChs-2] < x_o)) // x_u untere Schranke // x_o obere Schranke
                   {


                       //m_vTimes.push_back(m_qTime.elapsed());
                        emit sendByte(1);
                        msleep(10);
                        emit sendByte(0);
                        t_bFound = true;
                   }

                   m_pData.pop_front();
               }
               m_qMutex.unlock();

           }
    // Ende Zeitmessung alpha zeitmessung

#endif

#ifdef BUFFERX1


    while(m_bIsRunning)
    {

        emit sendByte(1);
        msleep(10);
        emit sendByte(0);
        msleep(500);

    }



#endif























// //Beginn Zeitmessung

//    m_pData.clear();


//    bool t_bFound = false;

//    int count = 0;


//    while(m_bIsRunning)
//    {
//        m_qMutex.lock();

//        if(t_bFound && m_pData.size() > 0)
//        {
//            m_pData.clear();
//            ++count;

//            if(count > 2)
//            {
//                t_bFound = false;
//                count = 0;

//                emit sendByte(1);
//                m_qTime.start();
//            }

//        }

//        if(m_pData.size() > 0)
//        {
//            if(!t_bFound && m_pData.first()[m_iNumChs-2] > 1000)
//            {
//                //std::cout << "Time elapsed: " << m_qTime.elapsed() << std::endl;

//                m_vTimes.push_back(m_qTime.elapsed());
//                emit sendByte(0);

//                t_bFound = true;
//            }

//            m_pData.pop_front();
//        }
//        m_qMutex.unlock();
//    }


//// Ende Zeitmessung

//    double v_old = 0;

//    while (true)
//    {
//        /* Dispatch the inputs */
//        double v = m_pDataSingleChannel->pop();

//        double diff = v_old - v;

//        m_pTriggerOutput->data()->setValue(diff);

//        if(diff > 20)
//        {
//            //ToDo: Implement your algorithm here

//        }



//        v_old  = v;




//    }

//    // ////////////////////////////
//    int count = 0;
//    double v = 0;

//    while (true)
//    {
//        //ToDo: Implement your algorithm here

//        if( (count % 5 == 0) )
//        {
//            v = count % 2;
//            //m_pTriggerOutput->data()->setValue(v);

//            if ( v == 1)
//            {
//                emit sendByte(1);
// //                m_pSerialPort->m_digchannel.replace(0,1); // select 1st digital channel
// //                m_pSerialPort->encodedig();             // encode signal to m_data
// //                m_pSerialPort->sendData(m_pSerialPort->m_data);

//            }
//            else if (v == 0)
//            {
//                emit sendByte(0);
// //                m_pSerialPort->m_digchannel.replace(0,0); // select 1st digital channel
// //                m_pSerialPort->encodedig();             // encode signal to m_data
// //                m_pSerialPort->sendData(m_pSerialPort->m_data);
//            }

//        }

//        msleep((1.0/256.0)*10000.0);
//        ++count;
//    }


}



//*************************************************************************************************************

void TriggerControl::sendByteTo(int value)
{
    if (value == 0)
    {
        m_pSerialPort->m_digchannel.replace(9,0); // select 1st digital channel
        m_pSerialPort->encodedig();             // encode signal to m_data
        m_pSerialPort->sendData(m_pSerialPort->m_data);
    }
    else if (value == 1)
    {
        m_pSerialPort->m_digchannel.replace(9,1); // select 1st digital channel
        m_pSerialPort->encodedig();             // encode signal to m_data
        m_pSerialPort->sendData(m_pSerialPort->m_data);
    }

}

//*************************************************************************************************************

void TriggerControl::byteReceived()
{
    m_isReceived = true;
}
