//=============================================================================================================
/**
 * @file     lsladapter.h
 * @author   Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke. All rights reserved.
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
 * @brief    Contains the declaration of the LSLAdapter class.
 *
 */

#ifndef LSLADAPTER_H
#define LSLADAPTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter_global.h"

#include <scShared/Plugins/abstractsensor.h>

#include <lsl_cpp.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QThread>
#include <QFutureWatcher>
#include <QMutex>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QListWidgetItem;

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE LSLADAPTERPLUGIN
//=============================================================================================================

namespace LSLADAPTERPLUGIN
{

//=============================================================================================================
// LSLADAPTERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class LSLAdapterProducer;

//=============================================================================================================
/**
 * The LSL class deals with the LSL library (labstreaminglayer)
 */
class LSLADAPTERSHARED_EXPORT LSLAdapter : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "lsladapter.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

public:
    //=========================================================================================================
    /**
     * Constructs LSLAdapter.
     */
    LSLAdapter();

    //=========================================================================================================
    /**
     * Destroys LSL.
     */
    virtual ~LSLAdapter();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;

    virtual void init();

    virtual void unload();

    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the LSL adapter by stopping its producer
     */
    virtual bool stop();

    virtual QWidget* setupWidget();

    virtual inline AbstractPlugin::PluginType getType() const;

    virtual inline QString getName() const;

    virtual QString getBuildInfo();

public slots:
    //=========================================================================================================
    /**
     * This is called by the UI, whenever the user wants to manually refresh the list of available LSL streams.
     */
    void onRefreshAvailableStreams();

    //=========================================================================================================
    /**
     * This is called by the UI, whenever the user has changed the stream to connect to.
     */
    void onStreamSelectionChanged(const lsl::stream_info& newStream);

    //=========================================================================================================
    /**
     * This is called by the UI, whenever the user has changed the desired output block size.
     */
    void onBlockSizeChanged(const int newBlockSize);

protected:
    //=========================================================================================================
    /**
     * The LSLAdapter has an empty run method, as all of the work is done in the producer.
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

private slots:
    //=========================================================================================================
    /**
     * This is called by the QFutureWatcher, indicating that the background scanning is complete.
     */
    void onLSLStreamScanReady();

private:
    //=========================================================================================================
    /**
     * Helper function for getting a list of LSL streams that fulfill the current filtering settings.
     */
    static QVector<lsl::stream_info> scanAvailableLSLStreams();

    //=========================================================================================================
    /**
     * Helper function that fills the FiffInfo member based on an LSL stream info.
     */
    void prepareFiffInfo(const lsl::stream_info& stream);

    //=========================================================================================================
    /**
     * Helper function: apparently LSL does not have an '==' operator where one side is const, so this function compares the UIDs instead.
     */
    inline static bool contains(const QVector<lsl::stream_info>& v, const lsl::stream_info& s);

    //=========================================================================================================
    /**
     * Helper function: simple validity check for stream infos (sometimes the resolved stream infos are empty)
     */
    inline static bool isValid(const lsl::stream_info& s);

    // fiff info / data output
    float                                           m_fSamplingFrequency;
    int                                             m_iNumberChannels;
    int                                             m_iOutputBlockSize;
    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA;

    // LSL stream management
    QFutureWatcher<QVector<lsl::stream_info> >      m_updateStreamsFutureWatcher;
    QVector<lsl::stream_info>                       m_vAvailableStreams;
    lsl::stream_info                                m_currentStream;
    bool                                            m_bHasValidStream;

    // producer management
    QThread                                         m_producerThread;
    LSLAdapterProducer*                             m_pProducer;

signals:
    //=========================================================================================================
    /**
     * This is emitted in order to tell the UI that the list of available LSL streams has been updated.
     *
     * @param[in] vStreamInfos Vector of available LSL streams.
     * @param[in] currentStream The LSL stream that the Adapter would currently connect to (upon start).
     */
    void updatedAvailableLSLStreams(const QVector<lsl::stream_info>& vStreamInfos, const lsl::stream_info& currentStream);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline SCSHAREDLIB::AbstractPlugin::PluginType LSLAdapter::getType() const
{
    return _ISensor;
}

//************************************************************************************************************

inline QString LSLAdapter::getName() const
{
    return "LSL";
}

//************************************************************************************************************

inline bool LSLAdapter::contains(const QVector<lsl::stream_info>& v, const lsl::stream_info& s)
{
    bool result = false;
    for(const auto& s2 : v)
        result = result | (s2.uid() == s.uid());
    return result;
}

//************************************************************************************************************

inline bool LSLAdapter::isValid(const lsl::stream_info& s)
{
    // stream with nonempty ID and name should be valid
    return (s.uid().empty() == false && s.name().empty() == false);
}
} // NAMESPACE

#endif // LSLADAPTER_H
