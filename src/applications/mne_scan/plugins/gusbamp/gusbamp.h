//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the declaration of the GUSBAmp class.
 */

#ifndef GUSBAMP_H
#define GUSBAMP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_global.h"
#include <scShared/Plugins/abstractsensor.h>
#include <utils/generics/circularbuffer.h>
#include <scMeas/realtimemultisamplearray.h>
#include <fiff/fiff.h>

#include "FormFiles/gusbampsetupwidget.h"
#include "FormFiles/gusbampsetupprojectwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

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
 * GUSBAmp...
 *
 * @brief The GUSBAmp class provides an EEG connector for the gTec USBAmp device.
 */
class GUSBAMPSHARED_EXPORT GUSBAmp : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "gusbamp.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class GUSBAmpProducer;
    friend class GUSBAmpSetupWidget;
    friend class GUSBAmpSetupProjectWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a GUSBAmp.
     */
    GUSBAmp();

    //=========================================================================================================
    /**
     * Destroys the GUSBAmp.
     */
    virtual ~GUSBAmp();

    //=========================================================================================================
    /**
     * building all setting for the FIFF-data-stream
     */
    void setUpFiffInfo();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;

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
     * Starts the GUSBAmp by starting the GUSBAmp's thread.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the GUSBAmp by stopping the GUSBAmp's thread.
     */
    virtual bool stop();

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

    //=========================================================================================================
    /**
     * returns the type of the plug in
     */
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
     * returns the name of the plugin
     */
    virtual QString getName() const;

    //=========================================================================================================
    /**
     * setups the widget
     */
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
     * splits the recorded FIFF file
     */
    void splitRecordingFile();

    virtual QString getBuildInfo();

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

private:
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr    m_pRTMSA_GUSBAmp;               /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<GUSBAmpSetupProjectWidget>                                   m_pGUSBampSetupProjectWidget;   /**< Widget for setup the project file*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_float>                       m_pCircularBuffer;    /**< Holds incoming raw data.*/

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/
    QSharedPointer<GUSBAmpProducer>     m_pGUSBAmpProducer;                 /**< the GUSBAmpProducer.*/
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                        /**< Fiff measurement info.*/

    std::vector<QString>        m_vSerials;                 /**< vector of all Serials (the first one is the master). */
    int                         m_iSampleRate;              /**< the sample rate in Hz (see documentation of the g.USBamp API for details on this value and the NUMBER_OF_SCANS!)*/
    int                         m_iSamplesPerBlock;         /**< The samples per block defined by the user via the GUI. */
    UCHAR                       m_iNumberOfChannels;        /**< the channels that should be acquired from each device. */
    std::vector<int>            m_viSizeOfSampleMatrix;     /**< vector including the size of the two dimensional sample Matrix. */
    std::vector<int>            m_viChannelsToAcquire;      /**< vector of the calling numbers of the channels to be acquired. */
    Eigen::RowVectorXd          m_cals;
};
} // NAMESPACE

#endif // GUSBAMP_H
