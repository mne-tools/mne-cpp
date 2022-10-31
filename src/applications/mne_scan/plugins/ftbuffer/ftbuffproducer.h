//=============================================================================================================
/**
 * @file     ftbuffproducer.h
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the FtBuffProducer class.
 *
 */

#ifndef FTBUFFPRODUCER_H
#define FTBUFFPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftconnector.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace FTBUFFERPLUGIN {

//=============================================================================================================
// FTBUFFERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class FtBuffer;
class FtConnector;

//=============================================================================================================
/**
 * FtBuffProducer
 *
 * Handles communication between FtConnector and FtBuffer.
 * Meant to be run in a separate thread, all communication is done through slots and signals.
 * Holds an instance of FtConnector as a member.
 *
 * @brief Handles communication and data transfer between FtConnector and FtBuffer
 */
class FtBuffProducer : public QObject
{
    Q_OBJECT

    friend class FtBuffer;
    friend class FtConnector;
    friend class FtBufferSetupWidget;

public:
    //=========================================================================================================
    /**
     * Creates instance of FtBuffProducer that holds a poiter to an instance of FtBuffer
     */
    FtBuffProducer(FtBuffer* pFtBuffer);

    //=========================================================================================================
    /**
     * Destroys an instance of FtBuffProducer
     */
    ~FtBuffProducer();

    //=========================================================================================================
    /**
     * Disconnects the member FtConnector from the buffer it is connected to
     */
    bool disconnectFromBuffer();

public slots:
    //=========================================================================================================
    /**
     * Runs the producer calss run() function
     */
    void doWork();

    //=========================================================================================================
    /**
     * Connects to the buffer at a given address. Tries to setup buffer output
     *
     * @param[in] addr  IP address where the buffer is hosted.
     * @param[in] port  Port where the buffer is hosted.
     */
    void connectToBuffer(QString addr,
                         int port);

protected:
    //=========================================================================================================
    /**
     * Loops continuously to aquire new data from FtConnector and send to FtBuffer
     */
    virtual void runMainLoop();

signals:
    //=========================================================================================================
    /**
     * Sends new buffer data formatted as an Eigen Matrix
     *
     * @param[in] matData   Formated data from buffer.
     */
    void newDataAvailable(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * Sends whether connection parameters have been set.
     *
     * @param[in] connection    Wheterher connection parameters have been set.
     */
    void connecStatus(bool connection);

private:
    FtBuffer*                       m_pFtBuffer;                /**< Pointer to FtBuffer that created this object. Destination of collected data. */

    FtConnector*                    m_pFtConnector;             /**< FtConnectr object that interfaces with buffer and gets buffer data. */

};

} // namespace

#endif // FTBUFFPRODUCER_H
