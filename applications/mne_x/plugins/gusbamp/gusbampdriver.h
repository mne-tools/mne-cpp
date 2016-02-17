//=============================================================================================================
/**
* @file     gusbampdriver.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the GUSBAmpdriver class. This class implements the basic communication between MNE-X and a GUSBAmp Refa device
*
*/





#ifndef GUSBAMPDRIVER_H
#define GUSBAMPDRIVER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Windows.h>        //windows.h-library for LPSTR-,UCHAR-, and HANDLE-files
#include <deque>
#include <stdarg.h>
#include "gtec_gUSBamp.h"
//#include "ringbuffer.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtCore>
#include <QtCore/QCoreApplication>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GUSBAmpPlugin
//=============================================================================================================

namespace GUSBAmpPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace std;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmpProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================


//=============================================================================================================
/**
* GUSBAmpDriver
*
* @brief The GUSBAmpDriver class provides real time data acquisition of EEG data with a GUSBAmp device.
*/
class GUSBAmpDriver
{
private:

//device-settings
    vector<LPSTR>       m_vsSerials;                //specify the serial number of the device used as master
    int                 m_numDevices;               //number of connected devices (master and slaves)
    deque<LPSTR>        m_callSequenceSerials;      //list of the call sequence (master must be the last device in the call sequence)
    deque<HANDLE>       m_openedDevicesHandles;     //list of handles in the order of the opened devices
    deque<HANDLE>       m_callSequenceHandles;      //list of handles in the order of the opened devices
    int                 m_SLAVE_SERIALS_SIZE;       //the number of slave serials specified in slaveSerials
    int                 m_SAMPLE_RATE_HZ;           //the sample rate in Hz (see documentation of the g.USBamp API for details on this value and the NUMBER_OF_SCANS!)
    int                 m_NUMBER_OF_SCANS;          //the number of scans that should be received simultaneously (depending on the _sampleRate; see C-API documentation for this value!)
    UCHAR               m_NUMBER_OF_CHANNELS;       //the number of channels per device that should be acquired (must equal the size of the _channelsToAcquire array)
    UCHAR               m_channelsToAcquire[16];    //the channels that should be acquired from each device
    const BOOL          m_TRIGGER;                  //TRUE to acquire the trigger line in an additional channel
    UCHAR               m_mode;                     //use normal acquisition mode
    CHANNEL             m_bipolarSettings;          //don't use bipolar derivation (all values will be initialized to zero)
    REF                 m_commonReference;          //don't connect groups to common reference
    GND                 m_commonGround;             //don't connect groups to common ground
    const int           m_QUEUE_SIZE;               //the number of GT_GetData calls that will be queued during acquisition to avoid loss of data
    bool                m_isRunning;                //flag for data acquisition
//buffer
    int                 m_nPoints;                  //number of points which are received from one chanel simultaneously
    DWORD               m_bufferSizeBytes;          //Size of buffer
    DWORD               m_numBytesReceived;         //num of Bytes whicht are received during one measuring procedure
    BYTE***             m_buffers;                  //pointer to the buffer
    OVERLAPPED**        m_overlapped;               //storage in case of overlapping
//file writing and outputmatrix
    QString             m_filePath;                 //path of the file for data acquisition
    QString             m_sFileName;                //file name of the data-file
    QFile               m_file;                     //file to which data is written
    QTextStream         m_stream;                   //stream from m_buffers to m_file
    vector<int>         m_sizeOfMatrix;             //number of rows and collums of output matrix [rows collums]


public:
    //=========================================================================================================
    /**
    * Constructs a GUSBAmpDriver.
    *
    * @param [in]   pGUSBAmpProducer a pointer to the corresponding GUSBAmp Producer class.
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
    * @param [in]   MatrixXf the block sample values in form of a matrix.
    * @return       returns true if sample was successfully written to the input variable, false otherwise.
    */
    bool getSampleMatrixValue(MatrixXf& sampleMatrix);

    //=========================================================================================================
    /**
    * Initialise and starts device with the set parameters . After that getSampleMatrixValue() has to be started
    * immediatly and be executed continously. Otherwise there will be a buffer overrun.
    */
    bool initDevice();

    //=========================================================================================================
    /**
    * Uninitialise device.
    * @return       returns true if device was successfully uninitialised, false otherwise.
    */
    bool uninitDevice();

    //=========================================================================================================
    /**
    * Setting the adresses of the master amplifer and the slaves. The selections of the slaves are optional.
    *
    * @param[in]    list        Serial Number of the master device
    *
    * @return[out]              true if suceeded
    *
    */
    bool setSerials(vector<LPSTR> &list);

    //=========================================================================================================
    /**
    * Setting the sampling rate of the amplifier and defining the Number of Scans
    *
    * @param[in]    samplingRate    sampling rate of the amplifier in [Hz] possible settings for the sample rate are:
    *                               32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200 and 38400
    * @return[out]                  true if suceeded
    *
    */
    bool setSampleRate(int sampleRate);

    //=========================================================================================================
    /**
    * Setting the channels and the Number of channels
    *
    * @param[in]    channels        Vector which behold the values of Channels as integer. The values have to be
    *                               ascending and in number must not exceed 16
    *
    * @return[out]                  true if suceeded
    *
    */
    bool setChannels(vector<int> &channels);

    //=========================================================================================================
    /**
    * Setting the path of the file
    *
    * @param[in]    QString         QString which beholds the the path of the File in which data will be stored
    *
    * @return[out]                  true if suceeded
    *
    */
    bool setFilePath(QString FilePath);

    //=========================================================================================================
    /**
    * Getting the size of the Sample Matrix
    *
    * @return[out]                  vector with the size of the matrix. first value = columns. second value rows.
    *
    */
    vector<int> getSizeOfSampleMatrix(void);



protected:
    GUSBAmpProducer*       m_pGUSBAmpProducer;                /**< A pointer to the corresponding GUSBAmpProducer class.*/

};

} // NAMESPACE

#endif // GUSBAMPDRIVER_H
