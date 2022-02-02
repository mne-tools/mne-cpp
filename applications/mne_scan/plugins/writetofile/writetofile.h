//=============================================================================================================
/**
 * @file     writetofile.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>,
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel B Motta . All rights reserved.
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
 * @brief    Contains the declaration of the WriteToFile class.
 *
 */

#ifndef WRITETOFILE_H
#define WRITETOFILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "writetofile_global.h"

#include <utils/generics/circularbuffer.h>
#include <scShared/Plugins/abstractalgorithm.h>
#include <fiff/fifffilesharer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QAction>
#include <QFile>
#include <QTime>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
    class FiffStream;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
}

#define MAX_DATA_LEN    2000000000L

//=============================================================================================================
// DEFINE NAMESPACE WRITETOFILEPLUGIN
//=============================================================================================================

namespace WRITETOFILEPLUGIN
{

//=============================================================================================================
// WRITETOFILEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS WriteToFile
 *
 * @brief The WriteToFile class provides a tools to reduce noise of an incoming data stream. It then forwards the processed data to subsequent plugins.
 */
class WRITETOFILESHARED_EXPORT WriteToFile : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "writetofile.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

public:
    //=========================================================================================================
    /**
     * Constructs a WriteToFile.
     */
    WriteToFile();

    //=========================================================================================================
    /**
     * Destroys the WriteToFile.
     */
    ~WriteToFile();

    //=========================================================================================================
    /**
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the pugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Sets whether the plugin is to be in continuous save mode
     *
     * @param iState    state of checkbox - gets saved as bool 0 - false (not continuous), 1+ - true (continuous)
     */
    void setContinuous(int iState);

    //=========================================================================================================
    /**
     * Whether the plugin is set to continuous save mode
     *
     * @return true if set to continuous, false if not.
     */
    bool isContinuous();

private:
    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();

    //=========================================================================================================
    /**
     * Set the recording time in seconds.
     *
     * @param[in] time   the new recording time.
     */
    void setRecordingTimerChanged(int timeMSecs);

    //=========================================================================================================
    /**
     * Set the recording time active flag.
     *
     * @param[in] state   whether the recording should be used or not.
     */
    void setRecordingTimerStateChanged(bool state);

    //=========================================================================================================
    /**
     * Set the recording file name.
     *
     * @param[in] sFileName   the new file name.
     */
    void onFileNameChanged(const QString& sFileName);

    //=========================================================================================================
    /**
     * Starts or stops a file recording depending on the current recording state.
     */
    void toggleRecordingFile();

    //=========================================================================================================
    /**
     * Determines current file. And starts a new one.
     */
    void splitRecordingFile();

    //=========================================================================================================
    /**
     * change recording button.
     */
    void changeRecordingButton();

    //=========================================================================================================
    /**
     * Copies recording and sends it to shared file direcotry without stopping reccording
     *
     * @param[in] bChecked      Unused. Whether action that triggered this function was checked or unchecked
     */
    void clipRecording(bool bChecked);

    //=========================================================================================================
    /**
     * Prompts user to rename recent recording file/files.
     */
    void promptFileName();

    //=========================================================================================================
    /**
     * Attempts to rename files from most recent recording with input parameter sFileName.
     *
     * @param[in] sFileName     new name for save files.
     *
     * @return Returns true if all files were renamed, false if not.
     */
    bool renameRecording(const QString& sFileName);

    //=========================================================================================================
    /**
     * Renames a single file
     *
     * @param[in] sCurrentFileName      current file name.
     * @param[in] sNewFileName          new file name.
     *
     * @return Returns true if file was renamed, false if not.
     */
    bool renameSingleFile(const QString& sCurrentFileName, const QString& sNewFileName);

    //=========================================================================================================
    /**
     * Renames multiple files using input param as template and adds "-n" to file names to denote order.
     *
     * @param[in] sFileName     new template file name
     *
     * @return Returns true if all files were renamed, false if not.
     */
    bool renameMultipleFiles(const QString& sFileName);

    //=========================================================================================================
    /**
     * Deletes latest recording.
     */
    void deleteRecording();

    //=========================================================================================================
    /**
     * Displays pop up message with sText. Blocking.
     *
     * @param[in] sText     Text to be displayed.
     */
    void popUp(const QString& sText);

    //=========================================================================================================
    /**
     * Displays pop up message with sText and sInfoText. Returns response. Blocking.
     *
     * @param sText         Text to be displayed.
     * @param sInfoText     Text to be displayed.
     *
     * @return  Returns response as QMessageBox::No or QMessageBox::Yes.
     */
    int popUpYesNo(const QString& sText,
                   const QString& sInfoText);

    //=========================================================================================================
    void initGUI();

    //=========================================================================================================
    void initGUIActions();

    bool                                    m_bWriteToFile;                 /**< Flag for for writing the received samples to a file. Defined by the user via the GUI.*/
    bool                                    m_bUseRecordTimer;              /**< Flag whether to use data recording timer.*/
    bool                                    m_bContinuous;                  /**< Flag for whether to start plugin in continuous save mode */

    qint16                                  m_iBlinkStatus;                 /**< The blink status of the recording button.*/
    qint32                                  m_iSplitCount;                  /**< File split count. */
    int                                     m_iRecordingMSeconds;           /**< Recording length in mseconds.*/

    QMutex                                  m_mutex;                        /**< The threads mutex.*/

    QSharedPointer<FIFFLIB::FiffInfo>       m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<FIFFLIB::FiffStream>     m_pOutfid;                      /**< FiffStream to write to.*/

    QSharedPointer<QTimer>                  m_pUpdateTimeInfoTimer;         /**< timer to control remaining time. */
    QSharedPointer<QTimer>                  m_pBlinkingRecordButtonTimer;   /**< timer to control blinking recording button. */
    QSharedPointer<QTimer>                  m_pRecordTimer;                 /**< timer to control recording time. */

    QFile                                   m_qFileOut;                     /**< QFile for writing to fif file.*/
    QString                                 m_sRecordFileName;              /**< Current record file. */
    QTime                                   m_recordingStartedTime;         /**< The time when the recording started.*/

    QPointer<QAction>                       m_pActionRecordFile;            /**< start recording action. */
    QPointer<QAction>                       m_pActionClipRecording;

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                      m_pCircularBuffer;      /**< Holds incoming raw data. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr      m_pWriteToFileInput;   /**< The RealTimeMultiSampleArray of the WriteToFile input.*/

    Eigen::RowVectorXd                      m_mCals;                        /**< Row vector with channel calibration values. */

    FIFFLIB::FiffFileSharer                 m_FileSharer;                   /**< Handles copying recording file and saving copy to shared directory. */

    QStringList                             m_lFileNames;                   /**< List of file names of latest recording */
};
} // NAMESPACE

#endif // WRITETOFILE_H
