//=============================================================================================================
/**
 * @file     gusbampdriver.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Viktor Klueber, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the GUSBAmpdriver class. This class implements the basic communication between MNE Scan and a GUSBAmp Refa device
 *
 */

#ifndef GUSBAMPDRIVER_H
#define GUSBAMPDRIVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <gusbamp_global.h>
#include <Windows.h>        //windows.h-library for LPSTR-,UCHAR-, and HANDLE-files
#include <deque>
#include <stdarg.h>
#include <Eigen/Core>
#include "gtec_gUSBamp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtCore>
#include <QtCore/QCoreApplication>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmpProducer;

//=============================================================================================================
/**
 * GUSBAmpDriver
 *
 * @brief The GUSBAmpDriver class provides real time data acquisition of EEG data with a GUSBAmp device.
 */
class GUSBAMPSHARED_EXPORT GUSBAmpDriver
{
private:
    //=========================================================================================================
    /**
     * Refreshes the size-values of the ouput Matrix
     */
    void refreshSizeOutputMatrix(void);

    //device parameters
    std::vector<QString>    m_vsSerials;                /**< vector of serial number as String vector (first one is master). */
    std::vector<QByteArray> m_vbSerials;                /**< vector of serial number as ByteArray vector (first one is master). */
    std::vector<LPSTR>      m_vpSerials;                /**< pointer to the serial numbers . */
    int                     m_numDevices;               /**< number of connected devices (master and slaves). */
    std::deque<LPSTR>       m_callSequenceSerials;      /**< list of the call sequence (master must be the last device in the call sequence). */
    std::deque<HANDLE>      m_openedDevicesHandles;     /**< list of handles in the order of the opened devices. */
    std::deque<HANDLE>      m_callSequenceHandles;      /**< list of handles in the order of the opened devices. */

    int                 m_iSlaveSerialsSize;        /**< the number of slave serials specified in slaveSerials. */
    int                 m_iSampleRateHz;            /**< the sample rate in Hz (see documentation of the g.USBamp API for details on this value and the NUMBER_OF_SCANS!). */
    int                 m_iNumberOfScans;           /**< the number of scans that should be received simultaneously (depending on the _sampleRate; see C-API documentation for this value!). */
    UCHAR               m_chNumberOfChannels;       /**< the number of channels per device that should be acquired (must equal the size of the _channelsToAcquire array). */
    UCHAR*              m_channelsToAcquire;        /**< the channels that should be acquired from each device. */
    const BOOL          m_bTrigger;                 /**< TRUE to acquire the trigger line in an additional channel. */
    UCHAR               m_mode;                     /**< use normal acquisition mode. */
    CHANNEL             m_bipolarSettings;          /**< don't use bipolar derivation (all values will be initialized to zero). */
    REF                 m_commonReference;          /**< don't connect groups to common reference. */
    GND                 m_commonGround;             /**< don't connect groups to common ground. */
    const int           m_QUEUE_SIZE;               /**< the number of GT_GetData calls that will be queued during acquisition to avoid loss of data. */
    bool                m_isRunning;                /**< flag for data acquisition. */

    //buffer
    int                 m_nPoints;                  /**< number of points which are received from one channel simultaneously. */
    DWORD               m_bufferSizeBytes;          /**< Size of buffer in Bytes. */
    DWORD               m_numBytesReceived;         /**< num of Bytes whicht are received during one measuring procedure. */
    BYTE***             m_buffers;                  /**< pointer to the buffer. */
    OVERLAPPED**        m_overlapped;               /**< storage in case of overlapping. */
    std::vector<int>    m_sizeOfMatrix;             /**< number of rows and column of output matrix [rows columns]. */

public:
    //=========================================================================================================
    /**
     * Constructs a GUSBAmpDriver.
     *
     * @param[in]  pGUSBAmpProducer a pointer to the corresponding GUSBAmp Producer class.
     */
    GUSBAmpDriver(GUSBAmpProducer* pGUSBAmpProducer);

    //=========================================================================================================
    /**
     * Destroys the GUSBAmpDriver.
     */
    ~GUSBAmpDriver();

    //=========================================================================================================
    /**
     * Get sample from the device in form of a mtrix.
     * @param[in]  MatrixXf    the block sample values in form of a matrix.
     *
     * @return                   returns true if sample was successfully written to the input variable, false otherwise.
     */
    bool getSampleMatrixValue(Eigen::MatrixXf& sampleMatrix);

    //=========================================================================================================
    /**
     * Initialise and starts device with the set parameters . After that getSampleMatrixValue() has to be started
     * immediatly and be executed continously. Otherwise a buffer overrun will occur.
     *
     * @return       returns true if succeeded.
     */
    bool initDevice();

    //=========================================================================================================
    /**
     * Uninitialise device.
     *
     * @return       returns true if device was successfully uninitialised, false otherwise.
     */
    bool uninitDevice();

    //=========================================================================================================
    /**
     * Setting the adresses of the master amplifer and the slaves. The selections of the slaves are optional.
     *
     * @param[in]   list        list of serial numbers of the devices. Master is first serialnumber in the list.
     *
     * @return                   true if executed successfully, false otherwise.
     *
     */
    bool setSerials(std::vector<QString> &list);

    //=========================================================================================================
    /**
     * Setting the sampling rate of the amplifier and defining the Number of Scans
     *
     * @param[in]   samplingRate    sampling rate of the amplifier in [Hz] possible settings for the sample rate are:.
     *                               32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200 and 38400
     *
     * @return                       true if executed successfully, false otherwise.
     *
     */
    bool setSampleRate(int sampleRate);

    //=========================================================================================================
    /**
     * Setting the channels and the Number of channels
     *
     * @param[in]   channels        Vector which behold the values of Channels as integer. The values have to be.
     *                               ascending and in number must not exceed 16
     *
     * @return                       true if executed successfully, false otherwise.
     *
     */
    bool setChannels(std::vector<int> &channels);

    //=========================================================================================================
    /**
     * Setting Flag for Filewriting
     *
     * @param[in]   doFileWriting   Boolian, which indicates whether Filewriting should be done or not.
     *
     * @return                       true if executed successfully, false otherwise.
     *
     */
    bool setFileWriting(bool doFileWriting);

    //=========================================================================================================
    /**
     * Setting the path of the file
     *
     * @param[in]   QString         QString which beholds the the path of the File in which data will be stored.
     *
     * @return                       true if executed successfully, false otherwise.
     *
     */
    bool setFilePath(QString FilePath);

    //=========================================================================================================
    /**
     * Getting the size of the Sample Matrix
     *
     * @return                       vector which beholds the size of the matrix. first value refers to columns.
     *
     */
    std::vector<int> getSizeOfSampleMatrix(void);

protected:
    GUSBAmpProducer*       m_pGUSBAmpProducer;      /**< A pointer to the corresponding GUSBAmpProducer class.*/
};
} // NAMESPACE

#endif // GUSBAMPDRIVER_H
