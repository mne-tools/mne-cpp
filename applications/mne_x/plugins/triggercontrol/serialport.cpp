//=============================================================================================================
/**
* @file     serialport.cpp
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
* @brief    Contains the implementation of the SerialPort class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "serialport.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TriggerControlPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SerialPort::SerialPort()
{
    initSettings();
    initPort();

    m_digchannel.resize(22);
    for (int i = 0; i < m_digchannel.size(); i++)
    {
        m_digchannel.replace(i,0);
    }

    //for (int i = 0; i < digchannel.size(); i++)
    //std::cout << digchannel[i] << std::endl;
    m_motor = 1;
    m_analval = 0;


    m_InAnChannelVal.resize(16);
    for (int i = 0; i < m_InAnChannelVal.size(); i++)
    {
        m_InAnChannelVal.replace(i,0);
    }

    m_InActiveDig.resize(22);
    for (int i = 0; i < m_InActiveDig.size(); i++)
    {
        m_InActiveDig.replace(i,0);
    }


    //connect(this->m_qSerialPort, SIGNAL(readyRead()), this, SLOT(SerialPort::readData()));   // if data available, read
    //connect(&m_qSerialPort, &readyRead, this, &SerialPort::readData);

}



//*************************************************************************************************************

SerialPort::~SerialPort()
{

}


//*************************************************************************************************************

void SerialPort::close()
{
    m_qSerialPort.close();
    std::cout << "port geschlossen" << std::endl;
}

//*************************************************************************************************************

void SerialPort::decodeana(QByteArray &t_incomingArray)
{

    int AnChannel=0;
    int AnValue=0;

    t_incomingArray[0]=t_incomingArray[0] << 2;     // left shift to pop the type info (11)

// decode channel
         if (t_incomingArray.at(0) == 0x00) {AnChannel=1;}
    else if (t_incomingArray.at(0)==0x10) {AnChannel=2;}
    else if (t_incomingArray.at(0)==0x20) {AnChannel=3;}
    else if (t_incomingArray.at(0)==0x40) {AnChannel=4;}
    else if (t_incomingArray.at(0)==0x80) {AnChannel=5;}
    else if (t_incomingArray.at(0)==0x30) {AnChannel=6;}
    else if (t_incomingArray.at(0)==0x50) {AnChannel=7;}
    else if (t_incomingArray.at(0)==0x90) {AnChannel=8;}
    else if (t_incomingArray.at(0)==0x60) {AnChannel=9;}
    else if (t_incomingArray.at(0)==0xA0) {AnChannel=10;}
    else if (t_incomingArray.at(0)==0xC0) {AnChannel=11;}
    else if (t_incomingArray.at(0)==0x70) {AnChannel=12;}
    else if (t_incomingArray.at(0)==0xB0) {AnChannel=13;}
    else if (t_incomingArray.at(0)==0xD0) {AnChannel=14;}
    else if (t_incomingArray.at(0)==0xE0) {AnChannel=15;}
    else if (t_incomingArray.at(0)==0xF0) {AnChannel=16;}
    else {std::cout << "Error during analog channel selection" << std::endl;}



// decode channel value
    t_incomingArray[1]=t_incomingArray[1] >> 2;
    t_incomingArray[2]=t_incomingArray[2] >> 2;
    t_incomingArray[3]=t_incomingArray[3] >> 2;

    if ((t_incomingArray[1]&0x08) == 0x08) {AnValue=AnValue+32768;}   // 0000 1000
    if ((t_incomingArray[1]&0x04) == 0x04) {AnValue=AnValue+16384;}   // 0000 0100
    if ((t_incomingArray[1]&0x02) == 0x02) {AnValue=AnValue+8192;}    // 0000 0010
    if ((t_incomingArray[1]&0x01) == 0x01) {AnValue=AnValue+4096;}    // 0000 0001
    if ((t_incomingArray[2]&0x20) == 0x20) {AnValue=AnValue+2048;}    // 0010 0000
    if ((t_incomingArray[2]&0x10) == 0x10) {AnValue=AnValue+1024;}    // 0001 0000
    if ((t_incomingArray[2]&0x08) == 0x08) {AnValue=AnValue+512;}     // 0000 1000
    if ((t_incomingArray[2]&0x04) == 0x04) {AnValue=AnValue+256;}     // 0000 0100
    if ((t_incomingArray[2]&0x02) == 0x02) {AnValue=AnValue+128;}     // 0000 0010
    if ((t_incomingArray[2]&0x01) == 0x01) {AnValue=AnValue+64;}
    if ((t_incomingArray[3]&0x20) == 0x20) {AnValue=AnValue+32;}
    if ((t_incomingArray[3]&0x10) == 0x10) {AnValue=AnValue+16;}
    if ((t_incomingArray[3]&0x08) == 0x08) {AnValue=AnValue+8;}
    if ((t_incomingArray[3]&0x04) == 0x04) {AnValue=AnValue+4;}
    if ((t_incomingArray[3]&0x02) == 0x02) {AnValue=AnValue+2;}
    if ((t_incomingArray[3]&0x01) == 0x01) {AnValue=AnValue+1;}

    std::cout << "Analoger Channel: " << AnChannel <<  " | Analogerwert:" << AnValue << std::endl;

    m_InAnChannelVal[AnChannel-1] = AnValue;

}

//*************************************************************************************************************

void SerialPort::decodedig(QByteArray &t_incomingArray)
{


// decode channel 1-6

    if ((t_incomingArray.at(3)&0x04) == 0x04) m_InActiveDig[0] = 1;              // 0000 0100
    else m_InActiveDig[0] = 0;

    if ((t_incomingArray.at(3)&0x08) == 0x08) m_InActiveDig[1] = 1;            // 0000 1000
    else m_InActiveDig[1] = 0;

    if ((t_incomingArray.at(3)&0x10) == 0x10) m_InActiveDig[2] = 1;            // 0001 0000
    else m_InActiveDig[2] = 0;

    if ((t_incomingArray.at(3)&0x20) == 0x20) m_InActiveDig[3] = 1;            // 0010 0000
    else m_InActiveDig[3] = 0;

    if ((t_incomingArray.at(3)&0x40) == 0x40) m_InActiveDig[4] = 1;             // 0100 0000
    else m_InActiveDig[4] = 0;

    if ((t_incomingArray.at(3)&0x80) == 0x80) m_InActiveDig[5] = 1;             // 1000 0000
    else m_InActiveDig[5] = 0;


// decode channel 7 - 12

    if ((t_incomingArray.at(2)&0x04) == 0x4) m_InActiveDig[6] = 1;            // 0000 0100
    else m_InActiveDig[6] = 0;

    if ((t_incomingArray.at(2)&0x08) == 0x8) m_InActiveDig[7] = 1;            // 0000 1000
    else m_InActiveDig[7] = 0;

    if ((t_incomingArray.at(2)&0x10) == 0x10) m_InActiveDig[8] = 1;            // 0001 0000
    else m_InActiveDig[8] = 0;

    if ((t_incomingArray.at(2)&0x20) == 0x20) m_InActiveDig[9] = 1;            // 0010 0000
    else m_InActiveDig[9] = 0;

    if ((t_incomingArray.at(2)&0x40) == 0x40) m_InActiveDig[10] = 1;            // 0100 0000
    else m_InActiveDig[10] = 0;

    if ((t_incomingArray.at(2)&0x80) == 0x80) m_InActiveDig[11] = 1;            // 1000 0000
    else m_InActiveDig[11] = 0;

// decode channel 13 - 18

    if ((t_incomingArray.at(1)&0x04) == 0x4) m_InActiveDig[12] = 1;            // 0000 0100
    else m_InActiveDig[12] = 0;

    if ((t_incomingArray.at(1)&0x08) == 0x8) m_InActiveDig[13] = 1;            // 0000 1000
    else m_InActiveDig[13] = 0;

    if ((t_incomingArray.at(1)&0x10) == 0x10) m_InActiveDig[14] = 1;            // 0001 0000
    else m_InActiveDig[14] = 0;

    if ((t_incomingArray.at(1)&0x20) == 0x20) m_InActiveDig[15] = 1;            // 0010 0000
    else m_InActiveDig[15] = 0;


    if ((t_incomingArray.at(1)&0x40) == 0x40) m_InActiveDig[16] = 1;            // 0100 0000
    else m_InActiveDig[16] = 0;

    if ((t_incomingArray.at(1)&0x80) == 0x80) m_InActiveDig[17] = 1;            // 1000 0000
    else m_InActiveDig[17] = 0;

// decode channel 19 - 22

    if ((t_incomingArray.at(0)&0x04) == 0x4) m_InActiveDig[18] = 1;            // 0000 0100
    else m_InActiveDig[18] = 0;

    if ((t_incomingArray.at(0)&0x08) == 0x8) m_InActiveDig[19] = 1;            // 0000 1000
    else m_InActiveDig[19] = 0;

    if ((t_incomingArray.at(0)&0x10) == 0x10) m_InActiveDig[20] = 1;            // 0001 0000
    else m_InActiveDig[20] = 0;

    if ((t_incomingArray.at(0)&0x20) == 0x20) m_InActiveDig[21] = 1;            // 0010 0000
    else m_InActiveDig[21] = 0;


    for ( int i = 0; i<22;i++)
        std::cout << "Kanal: " << m_InActiveDig[i] << std::endl;
}

//*************************************************************************************************************

void SerialPort::encodeana()
{


    m_data.clear();


//denote control bytes
    m_data[0] = m_data[0]|0xC0;
    m_data[1] = m_data[1]|0x01;
    m_data[2] = m_data[2]|0x02;
    m_data[3] = m_data[3]|0x03;


// Motorauswahl
    //Channelkodierung
    //1:0000	2:0001	3:0010	4:0100	5:1000	6:0011	7:0101	8:1001	9:0110	10:1010	11:1100	12:0111	13:1011	14:1101	15:1110	16:1111

    //m_data[0] = m_data[0]|0x04;

    if (m_motor == 1)     {m_data[0] = m_data[0]|0x00;}     // 0000 0000   1. Motor
    else if (m_motor == 2){m_data[0] = m_data[0]|0x04;}     // 0000 0100   2. Motor
    else if (m_motor == 3){m_data[0] = m_data[0]|0x08;}     // 0000 1000   3. Motor
    else if (m_motor == 4){m_data[0] = m_data[0]|0x10;}     // 0001 0000   4. Motor
    else {std::cout << "Fehler bei Motorauswahl" << std::endl;}

// Konvertierung analoger Wert


    int i = 32768;				 				//i=2^15
    int j = 0;								//Variable zur Kennzeichnung des entsprechenden Bytes in m_data
    int k = 0;								//Variable zur Positionierung des entsprechenden Bits
    QByteArray posArray = 0;					//Hilfsarray zum Ansprechen der Bits in m_data an der jeweiligen Position
    posArray.resize(6);
    posArray.clear();

    posArray [0]= posArray [0]|0x80;
    posArray [1]= posArray [1]|0x40;
    posArray [2]= posArray [2]|0x20;
    posArray [3]= posArray [3]|0x10;
    posArray [4]= posArray [4]|0x08;
    posArray [5]= posArray [5]|0x04;



    while (i>=1)    							//Schleife zum Konvertieren des analogen Wertes
        {
        if (i>2048 && i<32769) {j=1; k=2;} else if (i>32 && i<2049) {j=2; k=0;} else {j=3;k=0;};
        while (j==1)
        {
            if (m_analval/i >= 1){
            m_data[j] = m_data[j]|posArray[k];
            m_analval = m_analval-i;
            i=i/2;}
            else {i=i/2;}
            if (i>2048 && i<32769) {j=1; k++;} else {j=0;}
        }
        while (j==2)
        {
            if (m_analval/i >= 1){
            m_data[j] = m_data[j]|posArray[k];
            m_analval = m_analval-i;
            i=i/2;}
            else {i=i/2;}
            if (i>32 && i<2049) {j=2; k++;} else {j=0;}
        }
        while (j==3)
        {
            if (m_analval/i >= 1){
            m_data[j] = m_data[j]|posArray[k];
            m_analval = m_analval-i;
            i=i/2;}
            else {i=i/2;}
            if (i>0 && i<33) {j=3; k++;} else {j=0;}
        }
        }

    std::cout << "Analog ausgelesen" << std::endl;


}

//*************************************************************************************************************

void SerialPort::encodedig()
{

    m_data.clear();

    //denote control bytes
    m_data[0] = m_data[0]|0x40;
    m_data[1] = m_data[1]|0x01;
    m_data[2] = m_data[2]|0x02;
    m_data[3] = m_data[3]|0x03;


    // evaluate chosen digital channels
    // 1 - 6

    if (m_digchannel.at(0) == 1) m_data[3] = m_data[3]|0x04;     // 0000 0100
    if (m_digchannel.at(1) == 1) m_data[3] = m_data[3]|0x08;     // 0000 1000
    if (m_digchannel.at(2) == 1) m_data[3] = m_data[3]|0x10;     // 0001 0000
    if (m_digchannel.at(3) == 1) m_data[3] = m_data[3]|0x20;     // 0010 0000
    if (m_digchannel.at(4) == 1) m_data[3] = m_data[3]|0x40;     // 0100 0000
    if (m_digchannel.at(5) == 1) m_data[3] = m_data[3]|0x80;     // 1000 0000

    // 7 - 12
    if (m_digchannel.at(6) == 1) m_data[2] = m_data[2]|0x04;     // 0000 0100
    if (m_digchannel.at(7) == 1) m_data[2] = m_data[2]|0x08;     // 0000 1000
    if (m_digchannel.at(8) == 1) m_data[2] = m_data[2]|0x10;     // 0001 0000
    if (m_digchannel.at(9) == 1) m_data[2] = m_data[2]|0x20;     // 0010 0000
    if (m_digchannel.at(10) == 1) m_data[2] = m_data[2]|0x40;     // 0100 0000
    if (m_digchannel.at(11) == 1) m_data[2] = m_data[2]|0x80;     // 1000 0000

    // 13 - 18
    if (m_digchannel.at(12) == 1) m_data[1] = m_data[1]|0x04;     // 0000 0100
    if (m_digchannel.at(13) == 1) m_data[1] = m_data[1]|0x08;     // 0000 1000
    if (m_digchannel.at(14) == 1) m_data[1] = m_data[1]|0x10;     // 0001 0000
    if (m_digchannel.at(15) == 1) m_data[1] = m_data[1]|0x20;     // 0010 0000
    if (m_digchannel.at(16) == 1) m_data[1] = m_data[1]|0x40;     // 0100 0000
    if (m_digchannel.at(17) == 1) m_data[1] = m_data[1]|0x80;     // 1000 0000

    // 19 - 22
    if (m_digchannel.at(18) == 1) m_data[0] = m_data[0]|0x04;     // 0000 0100
    if (m_digchannel.at(19) == 1) m_data[0] = m_data[0]|0x08;     // 0000 1000
    if (m_digchannel.at(20) == 1) m_data[0] = m_data[0]|0x10;     // 0001 0000
    if (m_digchannel.at(21) == 1) m_data[0] = m_data[0]|0x20;     // 0010 0000


qDebug() << "Digital ausgelesen" << endl;

    //writeData(m_data);
}

//*************************************************************************************************************

void SerialPort::initSettings()
{
    m_currentSettings.name = "";
    m_currentSettings.baudRate = QSerialPort::Baud115200;
    m_currentSettings.stringBaudRate = "115200";
    m_currentSettings.dataBits = QSerialPort::Data8;
    m_currentSettings.stringDataBits = "8";
    m_currentSettings.parity = QSerialPort::NoParity;
    m_currentSettings.stringParity = "None";
    m_currentSettings.stopBits = QSerialPort::OneStop;
    m_currentSettings.stringStopBits = "1";
    m_currentSettings.flowControl = QSerialPort::NoFlowControl;
    m_currentSettings.stringFlowControl = "None";
}

//*************************************************************************************************************

void SerialPort::initPort()
{
    QList<QSerialPortInfo> t_qListPortInfo = QSerialPortInfo::availablePorts();

    bool t_correctPort = false;

//    if(t_qListPortInfo.size() > 1)
//    {
//        m_currentSettings.name = t_qListPortInfo[1].portName();
//        qDebug() << t_qListPortInfo[1].description().toLatin1() << endl;
//        m_qSerialPort.setPortName( t_qListPortInfo[1].portName());
//    }

    for (int t_count = 0; t_count <= t_qListPortInfo.size();t_count++)
    {
        if (t_qListPortInfo[t_count].description() == "Silicon Labs CP210x USB to UART Bridge")
        {

            t_correctPort = true;
            m_currentSettings.name = t_qListPortInfo[t_count].portName();
            m_qSerialPort.setPortName( t_qListPortInfo[t_count].portName());
            break;
        }
    }

    if(t_correctPort)
        std::cout << "correct port was found" << std::endl;
    else
        std::cout << "correct port was not found" << std::endl;

}


//*************************************************************************************************************
void SerialPort::readData()
{
    QByteArray t_incomingArray = m_qSerialPort.readAll();

    if(((t_incomingArray[0]&0x03) == 0x00) && ((t_incomingArray[1]&0x03) == 0x01) && ((t_incomingArray[2]&0x03) == 0x02) && ((t_incomingArray[3]&0x03) == 0x03))
    {
        if ((t_incomingArray[0]&0xC0) == 0x40)
            decodedig(t_incomingArray);

        else if ((t_incomingArray[0]&0xC0) == 0xC0)
            decodeana(t_incomingArray);

        else
            std::cout << "Error while reading" << std::endl;
    }


}



//*************************************************************************************************************

bool SerialPort::open()
{
    bool success = false;

    // get current settings
    m_qSerialPort.setPortName(m_currentSettings.name);




 //   qDebug() << "noch nicht geöffnet" << endl;
    if (m_qSerialPort.open(QIODevice::ReadWrite))
    {
        std::cout << "geöffnet, ohne Konfigs" << std::endl;
        if (m_qSerialPort.setBaudRate(m_currentSettings.baudRate)
                && m_qSerialPort.setDataBits(m_currentSettings.dataBits)
                && m_qSerialPort.setParity(m_currentSettings.parity)
                && m_qSerialPort.setStopBits(m_currentSettings.stopBits)
                && m_qSerialPort.setFlowControl(m_currentSettings.flowControl))
        {
            std::cout << "geöffnet, mit:"
                      << " Name: " << m_currentSettings.name.toLatin1().data()
                      << ", BaudRate: " << m_currentSettings.stringBaudRate.toLatin1().data()
                      << ", Databits: " << m_currentSettings.stringDataBits.toLatin1().data()
                      << ", Parity: " << m_currentSettings.stringParity.toLatin1().data()
                      << ", FlowControl: " << m_currentSettings.stringFlowControl.toLatin1().data()  << std::endl;

            success = true;
        }
        else
        {    std::cout << "nicht geöffnet, Fehler mit KOnfigurationszuweisung" << std::endl;
             m_qSerialPort.close();

            success = false;
        }

    }
    else
    {
        std::cout << "nicht geöffnet, Fehler schon beim readwriteöffnen" << std::endl;

        success = false;
    }

    return success;
}


//*************************************************************************************************************


void SerialPort::sendData(const QByteArray &data)
{
        m_qSerialPort.write(data);
}




