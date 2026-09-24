//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natusproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Contains the definition of the NatusProducer class.
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
    m_pUdpSocket = QSharedPointer<QUdpSocket>(new QUdpSocket());
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
    float fNumberSamples, fNumberChannels;

    char cInfo[3*sizeof(float)];
    stream.readRawData(cInfo, sizeof(cInfo));

    float* fInfo = reinterpret_cast<float*>(cInfo);

    fNumberSamples = fInfo[1];
    fNumberChannels = fInfo[2];

//    // Print info about received data
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
