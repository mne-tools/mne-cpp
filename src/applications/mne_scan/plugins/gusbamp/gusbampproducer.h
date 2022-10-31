//=============================================================================================================
/**
 * @file     gusbampproducer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the GUSBAmpProducer class.
 *
 */

#ifndef GUSBAMPPRODUCER_H
#define GUSBAMPPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <gusbamp_global.h>
#include <utils/generics/circularbuffer.h>
#include <vector>
#include <Windows.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmp;
class GUSBAmpDriver;

//=============================================================================================================
/**
 * DECLARE CLASS EEGProducer
 *
 * @brief The EEGProducer class provides a EEG data producer for a given sampling rate.
 */
class GUSBAMPSHARED_EXPORT GUSBAmpProducer : public QThread
{
public:
    //=========================================================================================================
    /**
     * Constructs a GUSBAmpProducer.
     *
     * @param[in] pGUSBAmp a pointer to the corresponding GUSBAmp class.
     */
    GUSBAmpProducer(GUSBAmp* pGUSBAmp);

    //=========================================================================================================
    /**
     * Destroys the GUSBAmpProducer.
     */
    ~GUSBAmpProducer();

    //=========================================================================================================
    /**
     * Starts the GUSBAmpProducer by starting the producer's thread and initialising the device.
     *
     * @param[in] serials       string array of all Serial names.
     * @param[in] channels      int field of calling number of the channels to be acquired.
     * @param[in] sampleRate    sample Rate as an integer.
     * @param[in] filePath      string of the filepath where data will be stored.
     *
     */
    virtual void start(std::vector<QString> &serials, std::vector<int> channels, int sampleRate);

    //=========================================================================================================
    /**
     * Stops the GUSBAmpProducer by stopping the producer's thread.
     */
    void stop();

    //=========================================================================================================
    /**
     * @return           returns the size of the sample Matrix.
     */
    std::vector<int> getSizeOfSampleMatrix(void);

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

private:
    GUSBAmp*                        m_pGUSBAmp;            /**< A pointer to the corresponding GUSBAmp class.*/
    QSharedPointer<GUSBAmpDriver>   m_pGUSBAmpDriver;      /**< A pointer to the corresponding GUSBAmp driver class.*/

    int                 m_iSampRate;            /**< sample rate of the device. */
    QString             m_sFilePath;            /**< path of the file of written data. */
    std::vector<QString>     m_vSerials;             /**< vector with the serial numbers of the devices*/
    std::vector<int>         m_viSizeOfSampleMatrix; /**< size of the sample matrix [rows columns]. */
};
} // NAMESPACE

#endif // GUSBAMPPRODUCER_H
