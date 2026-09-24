//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     writetofile.h
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the WriteToFile class.
 */

#ifndef WRITETOFILE_H
#define WRITETOFILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "writetofile_global.h"

#include <utils/generics/circularbuffer.h>
#include <scShared/Plugins/abstractalgorithm.h>
#include <fiff/fiff_file_sharer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QAction>
#include <QFile>
#include <QElapsedTimer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TestWriteToFileStatus;

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

    friend class ::TestWriteToFileStatus;

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
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual AbstractPlugin::PluginType getType() const override;
    virtual QString getName() const override;
    virtual QWidget* setupWidget() override;
    virtual QString getBuildInfo() override;
    virtual QVariantMap getAttributes() const override;
    virtual void setAttributes(const QVariantMap& attributes) override;

    //=========================================================================================================
    /**
     * Returns a small status widget that visualizes the current recording state
     * (red recording dot + elapsed time + current file size). Caller takes ownership.
     * Returns nullptr if recording status is not applicable.
     */
    QWidget* getStatusWidget() override;

signals:
    //=========================================================================================================
    /**
     * Emitted approximately once per second while recording with a human readable
     * summary of the form "HH:MM:SS  12.3 MB".
     */
    void recordingStatus(const QString& sSummary);

    //=========================================================================================================
    /**
     * Emitted whenever recording starts (true) or stops (false).
     */
    void recordingActiveChanged(bool bActive);

public:

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
    void setContinuous(Qt::CheckState iState);

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
    virtual void run() override;

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
    QSharedPointer<QTimer>                  m_pStatusEmitTimer;             /**< 1 Hz timer that emits recordingStatus while recording.*/

    //=========================================================================================================
    /**
     * Emit a recordingStatus signal computed from the current QFile and elapsed time.
     * Public so widgets and tests can trigger an immediate refresh.
     */
private:
    void emitRecordingStatus();

    //=========================================================================================================
    /**
     * Format a byte count as a human readable string ("12.3 MB").
     */
    static QString formatBytes(qint64 iBytes);

    //=========================================================================================================
    /**
     * Format an elapsed time in milliseconds as HH:MM:SS.
     */
    static QString formatElapsed(qint64 iMSecs);

    QFile                                   m_qFileOut;                     /**< QFile for writing to fif file.*/
    QString                                 m_sRecordFileName;              /**< Current record file. */
    QElapsedTimer                           m_recordingStartedTime;         /**< The time when the recording started.*/

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
