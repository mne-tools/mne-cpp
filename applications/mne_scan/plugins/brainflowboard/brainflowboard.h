//=============================================================================================================
/**
 * @file     brainflowboard.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke, Lorenz Esch. All rights reserved.
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
 * @brief Contains the declaration of BrainFlowBoard class.
 *
 */

#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowboard_global.h"
#include <scShared/Interfaces/ISensor.h>
#include <scMeas/realtimemultisamplearray.h>
#include "board_shim.h"

namespace FIFFLIB {
    class FiffInfo;
}

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


//=============================================================================================================
/**
 * DECLARE CLASS BrainFlowBoard
 *
 * @brief The BrainFlowBoard class performs Data Acquisition using BrainFlow BoardController module.
 */
class BRAINFLOWBOARD_EXPORT BrainFlowBoard : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainflowboard.json")
    Q_INTERFACES(SCSHAREDLIB::ISensor)

public:
    BrainFlowBoard();
    virtual ~BrainFlowBoard();

    virtual QSharedPointer<IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
     * Releases streaming session.
     *
     * @param [in] bUseQmessage when called from destructor is false, when from button callback is true.
     */
    void releaseSession(bool bUseQmessage = true);

    //=========================================================================================================
    /**
     * Prepares streaming session.
     *
     * @param [in] params BrainFlow Input params object.
     * @param [in] sStreamerParams parameters for BrainFlow streamer.
     * @param [in] iBoardId BrainFlow Board Id.
     * @param [in] iDataType determines data type to use(eeg,emg....).
     *
     */
    void prepareSession(BrainFlowInputParams params, std::string sStreamerParams, int iBoardId, int iDataType);

    //=========================================================================================================
    /**
     * Configures board with a string.
     *
     * @param [in] sConfig string to send to a board.
     *
     */
    void configureBoard(std::string sConfig);
    void showSettings();

protected:
    virtual void run();

private:

    std::string m_sStreamerParams;
    int m_iBoardId;
    QSharedPointer<BoardShim> m_pBoardShim;
    int m_iNumChannels;
    int *m_pChannels;
    int m_iSamplingRate;
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pOutput;
    volatile bool m_bIsRunning;
    QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;
    QAction *m_pShowSettingsAction;
};

#endif // BRAINFLOWBOARD_H
