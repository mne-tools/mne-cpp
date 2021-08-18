//=============================================================================================================
/**
 * @file     eegosports.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the declaration of the EEGoSports class.
 *
 */

#ifndef EEGOSPORTS_H
#define EEGOSPORTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosports_global.h"

#include "eegosportsproducer.h"
#include "FormFiles/eegosportsimpedancewidget.h"
#include "FormFiles/eegosportssetupwidget.h"
#include "FormFiles/eegosportssetupprojectwidget.h"

#include <scShared/Plugins/abstractsensor.h>
#include <utils/generics/circularbuffer.h>
#include <fstream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

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
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

typedef unsigned long DWORD;

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif

//=============================================================================================================
// EEGOSPORTSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * EEGoSports.
 *
 * @brief The EEGoSports class provides an EEG connector.
 */
class EEGOSPORTSSHARED_EXPORT EEGoSports : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "eegosports.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class EEGoSportsProducer;
    friend class EEGoSportsImpedanceWidget;
    friend class EEGoSportsSetupWidget;
    friend class EEGoSportsSetupProjectWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a EEGoSports.
     */
    EEGoSports();

    //=========================================================================================================
    /**
     * Destroys the EEGoSports.
     */
    virtual ~EEGoSports();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<AbstractPlugin> clone() const;

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
    * Sets up the fiff info with the current data chosen by the user.
    */
    void setNumberOfChannels(int iNumberOfChannels,
                             int iNumberOfEEGChannels,
                             int iNumberOfBipolarChannels);

    //=========================================================================================================
    /**
    * Starts the EEGoSports by starting the thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the EEGoSports by stopping the thread.
     */
    virtual bool stop();

    //=========================================================================================================
    /**
     * Set/Add received samples to the circular buffer.
     */
    void setSampleData(Eigen::MatrixXd &matData);

    //=========================================================================================================

    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    virtual QString getBuildDateTime();

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
    void onUpdateCardinalPoints(const QString& sLPA,
                                double dLPA,
                                const QString& sRPA,
                                double dRPA,
                                const QString& sNasion,
                                double dNasion);

protected:
    //=========================================================================================================
    /**
    * Opens a dialog to setup the project to check the impedance values
    */
    void showImpedanceDialog();

    //=========================================================================================================
    /**
    * Opens a dialog to setup the project to check the impedance values
    */
    void showSetupProjectDialog();

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr    m_pRMTSA_EEGoSports;                    /**< The RealTimeSampleArray to provide the EEG data.*/
    QSharedPointer<EEGoSportsImpedanceWidget>                                   m_pEEGoSportsImpedanceWidget;           /**< Widget for checking the impedances*/
    QSharedPointer<EEGoSportsSetupProjectWidget>                                m_pEEGoSportsSetupProjectWidget;        /**< Widget for checking the impedances*/
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                      m_pCircularBuffer;              /**< Holds incoming raw data. */

    QString                             m_qStringResourcePath;              /**< The path to the EEG resource directory.*/

    int                                 m_iSamplingFreq;                    /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    int                                 m_iNumberOfChannels;                /**< The number of channels.*/
    int                                 m_iNumberOfEEGChannels;             /**< The number of EEG channels.*/
    int                                 m_iNumberOfBipolarChannels;         /**< The number of Bipolar channels.*/

    int                                 m_iSamplesPerBlock;                 /**< The samples per block defined by the user via the GUI.*/

    double                              m_dLPAShift;                        /**< The shift in m in to generate the LPA.*/
    double                              m_dRPAShift;                        /**< The shift in m in to generate the RPA.*/
    double                              m_dNasionShift;                     /**< The shift in m in to generate the Nasion.*/

    bool                                m_bWriteDriverDebugToFile;          /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                                m_bCheckImpedances;                 /**< Flag for checking the impedances of the EEG amplifier.*/
    bool                                m_bUseTrackedCardinalMode;          /**< Flag for using the tracked cardinal mode.*/
    bool                                m_bUseElectrodeShiftMode;           /**< Flag for using the electrode shift mode.*/

    QString                             m_sElcFilePath;                     /**< Holds the path for the .elc file (electrode positions). Defined by the user via the GUI.*/
    QString                             m_sCardinalFilePath;                /**< Holds the path for the .elc file holding the cardinals/fiducials (electrode positions). Defined by the user via the GUI.*/
    QString                             m_sLPA;                             /**< The electrode to take to function as the LPA.*/
    QString                             m_sRPA;                             /**< The electrode to take to function as the RPA.*/
    QString                             m_sNasion;                          /**< The electrode to take to function as the Nasion.*/

    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                        /**< Fiff measurement info.*/

    QSharedPointer<EEGoSportsProducer>  m_pEEGoSportsProducer;              /**< The EEGoSportsProducer.*/

    QMutex                              m_mutex;                            /**< Holds the threads mutex.*/
};
} // NAMESPACE

#endif // EEGOSPORTS_H
