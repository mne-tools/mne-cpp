/**
* @file     ftbuffproducer.h
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>

//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ftsrc/ftbuffclient.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace FTBUFFERPLUGIN {

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FtBuffer;

//=============================================================================================================

/**
 * FtBuffProducer
 *
 * Handles communication between FtBuffClient and FtBuffer.
 * Meant to be run in a separate thread, all communication is done through slots and signals.
 * Holds an instance of FtBuffClient as a member.
 *
 * @brief Handles communication and data transfer between FtBuffClient and FtBuffer
 */
class FtBuffProducer : public QObject
{
    Q_OBJECT

    friend class FtBuffer;
    friend class FtBuffClient;
    friend class FtBufferSetupWidget;

public:

    //=========================================================================================================
    /**
    * creates instance of FtBuffProducer that holds a poiter to an instance of FtBuffer
    *
    */
    FtBuffProducer(FtBuffer* pFtBuffer);

    //=========================================================================================================
    ~FtBuffProducer();

    //=========================================================================================================
    /**
    * Changes stored address and connects the member FtBuffClient to that address
    *
    * @brief connects buffer client to provided address
    */
    bool connectToBuffer(QString addr);

    //=========================================================================================================
    /**
    * @brief disconnects the member FtBuffClient from the buffer it is connected to
    */
    bool disconnectFromBuffer();

public slots:

    //=========================================================================================================
    /**
    * @brief doWork - runs run()
    */
    void doWork();

protected:

    //=========================================================================================================
    /**
    * runs getData() on a loop, emiting newDataAvailable with new data from FtuffClient
    *
    * @brief loops continuously to aquire new data from FtBuffClient and send to FtBuffer
    */
    virtual void run();

signals:

    //=========================================================================================================
    /**
    * @brief newDataAvailable - Sends new buffer data
    * @param matData - formated data from buffer
    */
    void newDataAvailable(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
    * @brief extendedHeaderChunks
    * @param chunkData
    */
    void extendedHeaderChunks(SimpleStorage chunkData);


    void bufferParameters(QPair<int,int> numChanandFreq);

private:

    FtBuffer*                       m_pFtBuffer;                /**< Pointer to FtBuffer that created this object. Destination of collected data */
    FtBuffClient*                   m_pFtBuffClient;            /**< FtBuffClient object that interfaces with buffer and gets buffer data */

    Eigen::MatrixXd                 m_matData;                  /**< Aquired buffer data that will be sent to FtBuffProduer */
    char*                           m_pTempAddress;             /**< Temporary storage for setting FtBuffClient address */
};

} // namespace

#endif // FTBUFFPRODUCER_H
