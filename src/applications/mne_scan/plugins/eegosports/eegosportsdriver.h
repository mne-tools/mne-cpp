//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     eegosportsdriver.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the EEGoSportsDriver class.
 */

#ifndef EEGOSPORTSDRIVER_H
#define EEGOSPORTSDRIVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fstream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace eemagine {
    namespace sdk {
        class stream;
        class amplifier;
    }
}

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
// EEGOSPORTSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class EEGoSportsProducer;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define EEGO_SDK_BIND_DYNAMIC // How to bind

//=============================================================================================================
/**
 * EEGoSportsDriver
 *
 * @brief  This class implements the basic communication between MNE Scan and a ANT EEGoSports device.
 */
class EEGoSportsDriver
{

public:
    //=========================================================================================================
    /**
     * Constructs a EEGoSportsDriver.
     *
     * @param[in] pEEGoSportsProducer a pointer to the corresponding EEGoSportsProducer class.
     */
    EEGoSportsDriver(EEGoSportsProducer* pEEGoSportsProducer);

    //=========================================================================================================
    /**
     * Destroys the EEGoSportsDriver.
     */
    ~EEGoSportsDriver();

    //=========================================================================================================
    /**
     * Get sample from the device in form of a mtrix.
     *
     * @param[in] MatrixXf the block sample values in form of a matrix.
     * @param[in, out] bool returns true if sample was successfully written to the input variable, false otherwise.
     */
    bool getSampleMatrixValue(Eigen::MatrixXd& sampleMatrix);

    //=========================================================================================================
    /**
     *  Initialise device.
     *
     *  @param[in] iNumberOfChannels number of channels specified by the user.
     *  @param[in] iSamplesPerBlock samples per block specified by the user.
     *  @param[in] iSamplingFrequency sampling frequency specified by the user.
     *  @param[in] bWriteDriverDebugToFile Flag for writing driver debug information to a file. Defined by the user via the GUI.
     *  @param[in] bMeasureImpedance Flag for measuring impedances.
     */
    bool initDevice(bool bWriteDriverDebugToFile,
                    bool bMeasureImpedance);

    //=========================================================================================================
    /**
     *  Start recording.
     *
     *  @param[in] iSamplesPerBlock samples per block specified by the user.
     *  @param[in] iSamplingFrequency sampling frequency specified by the user.
     *  @param[in] bMeasureImpedance Flag for measuring impedances.
     */
    bool startRecording(int iSamplesPerBlock,
                        int iSamplingFrequency,
                        bool bMeasureImpedance);

    //=========================================================================================================
    /**
     *  Uninitialise device.
     *
     *  @param[in, out] bool returns true if device was successfully uninitialised, false otherwise.
     */
    bool uninitDevice();

    //=========================================================================================================
    /**
     *  Get number of channels.
     */
    uint getNumberOfChannels();
    uint getNumberOfEEGChannels();
    uint getNumberOfBipolarChannels();

    //=========================================================================================================
    /**
     *  Get list of channel types.
     */
    QList<uint> getChannellist();

private:
    EEGoSportsProducer*         m_pEEGoSportsProducer;          /**< A pointer to the corresponding EEGoSportsProducer class.*/

    bool                        m_bInitDeviceSuccess;           /**< Flag which defines if the device initialisation was successfully.*/
    bool                        m_bStartRecordingSuccess;       /**< Flag which defines if the recording was started successfully.*/
    bool                        m_bDllLoaded;                   /**< Flag which defines if the driver DLL was loaded successfully.*/

    uint                        m_uiNumberOfChannels;           /**< The number of channels.*/
    uint                        m_uiNumberOfEEGChannels;        /**< The number of EEG channels.*/
    uint                        m_uiNumberOfBipolarChannels;    /**< The number of Bipolar channels.*/
    uint                        m_uiSamplingFrequency;          /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    uint                        m_uiSamplesPerBlock;            /**< The samples per block defined by the user via the GUI.*/
    bool                        m_bWriteDriverDebugToFile;      /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                        m_bMeasureImpedances;           /**< Flag for impedance measuring mode.*/

    QList<Eigen::VectorXd>      m_lSampleBlockBuffer;           /**< Buffer to store all the incoming smaples. This is the buffer which is getting read from.*/

    eemagine::sdk::stream*      m_pDataStream;                  /**< The EEG/Impedance data stream.*/
    eemagine::sdk::amplifier*   m_pAmplifier;                   /**< Interface to the amplifier.*/

    std::ofstream               m_outputFileStream;             /**< fstream for writing the driver debug informations to a txt file.*/
};
} // NAMESPACE

#endif // EEGOSPORTSDRIVER_H
