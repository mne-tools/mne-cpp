//=============================================================================================================
/**
 * @file     natusproducer.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the definition of the NatusProducer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natusproducer.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QDataStream>
#include <QScopedArrayPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NATUSPLUGIN;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NatusProducer::NatusProducer(int iBlockSize, int iChannelSize, QObject *parent)
: QObject(parent)
, m_iMatDataSampleIterator(1)
{
    //Init socket
    m_pUdpSocket = QSharedPointer<QUdpSocket>(new QUdpSocket(this));
    m_pUdpSocket->bind(QHostAddress::AnyIPv4, 50000);
    connect(m_pUdpSocket.data(), &QUdpSocket::readyRead,
            this, &NatusProducer::readPendingDatagrams);

    m_matData.resize(iChannelSize, iBlockSize);
    m_fSampleFreq = 0;
    m_fChannelSize = 0;
}

//=============================================================================================================

void NatusProducer::readPendingDatagrams()
{
    while (m_pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_pUdpSocket->receiveDatagram();
        //qDebug() << "Datagram on port 50000 from IP "<<datagram.senderAddress();
        processDatagram(datagram);
    }
}

//=============================================================================================================

void NatusProducer::processDatagram(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    QDataStream stream(data);

    // Read info
    float fPackageNumber, fNumberSamples, fNumberChannels;

    char cInfo[3*sizeof(float)];
    stream.readRawData(cInfo, sizeof(cInfo));

    float* fInfo = reinterpret_cast<float*>(cInfo);

    fPackageNumber = fInfo[0];
    fNumberSamples = fInfo[1];
    fNumberChannels = fInfo[2];

//    // Print info about received data
//    qDebug()<<"fPackageNumber "<<fPackageNumber;
//    qDebug()<<"fNumberSamples "<<fNumberSamples;
//    qDebug()<<"fNumberChannels "<<fNumberChannels;
//    qDebug()<<"data.size() "<<data.size();
//    qDebug()<<"data.size() "<<data.size();
//    qDebug()<<"data.size() "<<data.size();

    // Read actual data
    int iDataSize = int(fNumberSamples * fNumberChannels);
    QScopedArrayPointer<char> cData(new char[iDataSize*sizeof(float)]);
    //char *cData = new char[iDataSize*sizeof(float)];
    stream.readRawData(cData.data(), iDataSize*sizeof(float));

    float* fData = reinterpret_cast<float*>(cData.data());

    //Get data
    Eigen::MatrixXf matData;
    matData.resize(fNumberChannels, fNumberSamples);
    int itr = 0;
    for(int j = 0; j < fNumberSamples; ++j) {
        for(int i = 0; i < fNumberChannels; ++i) {
            matData(i,j) = fData[itr++]/10e06;
        }
    }

    if(m_iMatDataSampleIterator+matData.cols() <= m_matData.cols()) {
        m_matData.block(0, m_iMatDataSampleIterator, matData.rows(), matData.cols()) = matData.cast<double>();

        m_iMatDataSampleIterator += matData.cols();
    } else {
        m_matData.block(0, m_iMatDataSampleIterator, matData.rows(), m_matData.cols()-m_iMatDataSampleIterator) = matData.block(0, 0, matData.rows(), m_matData.cols()-m_iMatDataSampleIterator).cast<double>();

        m_iMatDataSampleIterator = 0;
    }

    //qDebug() << "m_iMatDataSampleIterator" << m_iMatDataSampleIterator;

    if(m_iMatDataSampleIterator == m_matData.cols()) {
        m_iMatDataSampleIterator = 0;
        //qDebug()<<"Emit data";
        MatrixXd matEmit = m_matData.cast<double>();
        emit newDataAvailable(matEmit);
    }
}
