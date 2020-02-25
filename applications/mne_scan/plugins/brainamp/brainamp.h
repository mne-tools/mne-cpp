//=============================================================================================================
/**
 * @file     brainamp.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the BrainAMP class.
 *
 */

#ifndef BRAINAMP_H
#define BRAINAMP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_global.h"

#include <fstream>

#include <scShared/Interfaces/ISensor.h>
#include <utils/generics/circularmatrixbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>
#include <Eigen/Geometry>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
    class FiffStream;
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMPProducer;
class BrainAMPSetupWidget;
class BrainAMPSetupProjectWidget;

//=============================================================================================================
/**
 * BrainAMP...
 *
 * @brief The BrainAMP class provides a EEG connector. In order for this plugin to work properly the driver dll "RTINST.dll" must be installed in the system directory. This dll is automatically copied in the system directory during the driver installtion of the TMSi Refa device.
 */
class BRAINAMPSHARED_EXPORT BrainAMP : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainamp.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class BrainAMPProducer;
    friend class BrainAMPSetupWidget;
    friend class BrainAMPImpedanceWidget;
    friend class BrainAMPSetupProjectWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a BrainAMP.
     */
    BrainAMP();

    //=========================================================================================================
    /**
     * Destroys the BrainAMP.
     */
    virtual ~BrainAMP();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    //=========================================================================================================
    /**
     * Sets up the fiff info with the current data chosen by the user.
     */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
     * Starts the BrainAMP by starting the tmsi's thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the BrainAMP by stopping the tmsi's thread.
     */
    virtual bool stop();

    //=========================================================================================================
    /**
     * Set/Add received samples to a QList.
     */
    void setSampleData(Eigen::MatrixXd &matRawBuffer);

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

protected slots:
    //=========================================================================================================
    /**
     * Update cardinal points
     *
     * @param[in] sLPA  The channel name to take as the LPA.
     * @param[in] dLPA  The amount (in m) to translate the LPA channel position on the z axis.
     * @param[in] sRPA  The channel name to take as the RPA.
     * @param[in] dRPA  The amount (in m) to translate the RPA channel position on the z axis.
     * @param[in] sNasion  The channel name to take as the Nasion.
     * @param[in] dNasion  The amount (in m) to translate the Nasion channel position on the z axis.
     */
    void onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion);

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

    //=========================================================================================================
    /**
     * Opens a dialog to setup the project to check the impedance values
     */
    void showSetupProjectDialog();

    //=========================================================================================================
    /**
     * Starts data recording
     */
    void showStartRecording();

    //=========================================================================================================
    /**
     * Implements blinking recording button
     */
    void changeRecordingButton();

    //=========================================================================================================
    /**
     * Checks if a dir exists
     */
    bool dirExists(const std::string& dirName_in);

private:
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> >  m_pRMTSA_BrainAMP;              /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<BrainAMPSetupProjectWidget>                                              m_pBrainAMPSetupProjectWidget;  /**< Widget for checking the impedances*/

    QSharedPointer<IOBUFFER::RawMatrixBuffer>     m_pRawMatrixBuffer_In;              /**< Holds incoming raw data.*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iSamplesPerBlock;                 /**< The number of channels defined by the user via the GUI.*/

    double                              m_dLPAShift;                        /**< The shift in m in to generate the LPA.*/
    double                              m_dRPAShift;                        /**< The shift in m in to generate the RPA.*/
    double                              m_dNasionShift;                     /**< The shift in m in to generate the Nasion.*/

    bool                                m_bWriteToFile;                     /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                m_bIsRunning;                       /**< Whether BrainAMP is running.*/
    bool                                m_bCheckImpedances;                 /**< Flag for checking the impedances of the EEG amplifier.*/
    bool                                m_bUseTrackedCardinalMode;          /**< Flag for using the tracked cardinal mode.*/
    bool                                m_bUseElectrodeShiftMode;           /**< Flag for using the electrode shift mode.*/

    std::ofstream                       m_outputFileStream;                 /**< fstream for writing the samples values to txt file.*/

    QString                             m_sOutputFilePath;                  /**< Holds the path for the sample output file. Defined by the user via the GUI.*/
    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QString                             m_sCardinalFilePath;                /**< Holds the path for the .elc file holding the cardinals/fiducials (electrode positions). Defined by the user via the GUI.*/
    QString                             m_sLPA;                             /**< The electrode to take to function as the LPA.*/
    QString                             m_sRPA;                             /**< The electrode to take to function as the RPA.*/
    QString                             m_sNasion;                          /**< The electrode to take to function as the Nasion.*/

    QFile                               m_fileOut;                          /**< QFile for writing to fif file.*/
    QSharedPointer<FIFFLIB::FiffStream> m_pOutfid;                          /**< QFile for writing to fif file.*/
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                        /**< Fiff measurement info.*/
    Eigen::RowVectorXd                  m_cals;

    QSharedPointer<BrainAMPProducer>    m_pBrainAMPProducer;                /**< the BrainAMPProducer.*/

    QMutex                              m_qMutex;                           /**< Holds the threads mutex.*/

    QAction*                            m_pActionSetupProject;              /**< shows setup project dialog */
    QAction*                            m_pActionStartRecording;            /**< starts to record data */
    QAction*                            m_pActionSetupStimulus;             /**< starts stimulus feature */

    QSharedPointer<QTimer>              m_pTimerRecordingChange;            /**< timer to control blinking of the recording icon */
    qint16                              m_iBlinkStatus;                     /**< flag for recording icon blinking */

    QList<Eigen::MatrixXd>              m_qListReceivedSamples;             /**< list with alle the received samples in form of differentley sized matrices. */

    QMutex                              m_mutex;

};

} // NAMESPACE

#endif // BRAINAMP_H
