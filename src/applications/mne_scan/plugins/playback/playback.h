//=============================================================================================================
/**
 * @file     playback.h
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the declaration of the Playback class.
 *
 */

#ifndef PLAYBACK_H
#define PLAYBACK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "playback_global.h"

#include <utils/generics/circularbuffer.h>
#include <scShared/Plugins/abstractsensor.h>
#include <fiff/fifffilesharer.h>

#include <memory>

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

namespace FIFFLIB{
    class FiffInfo;
    class FiffStream;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE PLAYBACKPLUGIN
//=============================================================================================================

namespace PLAYBACKPLUGIN
{

//=============================================================================================================
// PLAYBACKPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Playback
 *
 * @brief The Playback class provides a tools to reduce noise of an incoming data stream. It then forwards the processed data to subsequent plugins.
 */
class PLAYBACKSHARED_EXPORT Playback : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "playback.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

public:
    //=========================================================================================================
    /**
     * Constructs a Playback.
     */
    Playback();

    //=========================================================================================================
    /**
     * Destroys the Playback.
     */
    ~Playback();

    //=========================================================================================================
    /**
     * AbstractSensorfunctions
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

private:
    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();

    bool loadFiffRawData();

    void initConnector();

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pRTMSA_Playback;     /**< The RealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_float>   m_pCircularBuffer;      /**< Holds incoming raw data. */
    QString sourceFilePath;
    std::unique_ptr<QFile> sourceFile;
    FIFFLIB::FiffRawData rawData;

    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;                /**< Fiff measurement info.*/
    QSharedPointer<FIFFLIB::FiffDigitizerData>      m_pFiffDigData;             /**< Fiff Digitizer Data. */


public slots:
    void setSourceFile(QString filePath);
};
} // NAMESPACE

#endif // PLAYBACK_H
