//=============================================================================================================
/**
 * @file     brainflowboard.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Andrey Parfenov. All rights reserved.
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
 * @brief    Contains the declaration of the BrainFlowBoard class.
 *
 */

#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <vector>

#include "brainflowboard_global.h"

#include "board_shim.h"

#include <scShared/Plugins/abstractsensor.h>

#include <utils/buildtime.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE BRAINFLOWBOARDPLUGIN
//=============================================================================================================

namespace BRAINFLOWBOARDPLUGIN
{

//=============================================================================================================
// BRAINFLOWBOARDPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS BrainFlowBoard
 *
 * @brief The BrainFlowBoard class provides a plugin for connecting to BrainFlow devices.
 */
class BRAINFLOWBOARD_EXPORT BrainFlowBoard : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainflowboard.json")
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

public:
    BrainFlowBoard();
    virtual ~BrainFlowBoard();

    virtual QSharedPointer<AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    void setUpFiffInfo();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    void releaseSession(bool useQmessage = true);
    void prepareSession(BrainFlowInputParams params,
                        std::string streamerParams,
                        int boardId,
                        int dataType);
    void configureBoard(std::string config);
    void showSettings();

protected:
    virtual void run();

private:
    std::string         m_sStreamerParams;
    BoardShim*          m_pBoardShim;
    QAction*            m_pShowSettingsAction;
    int                 m_iBoardId;
    int                 m_uiSamplesPerBlock;            /**< The samples per block defined by the user via the GUI.*/
    std::vector<int>    m_vChannels;
    int                 m_iSamplingFreq;

    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pOutput;
    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;        /**< Fiff measurement info.*/
};
}

#endif // BRAINFLOWBOARD_H
