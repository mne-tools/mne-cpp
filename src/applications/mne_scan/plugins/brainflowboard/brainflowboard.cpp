//=============================================================================================================
/**
 * @file     brainflowboard.cpp
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
 * @brief    Contains the definition of the BrainFlowBoard class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMessageBox>

#include "data_filter.h"

#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"
#include "FormFiles/brainflowstreamingwidget.h"

#include <fiff/fiff_info.h>

#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINFLOWBOARDPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowBoard::BrainFlowBoard()
: m_pBoardShim(NULL)
, m_iBoardId((int)BoardIds::SYNTHETIC_BOARD)
, m_pOutput(NULL)
, m_iSamplingFreq(0)
, m_sStreamerParams("")
, m_uiSamplesPerBlock(100)
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_bProcessOutput(false)
{
    m_pShowSettingsAction = new QAction(QIcon(":/images/options.png"), tr("Streaming Settings"),this);
    m_pShowSettingsAction->setStatusTip(tr("Streaming Settings"));
    connect(m_pShowSettingsAction, &QAction::triggered, this, &BrainFlowBoard::showSettings);
    addPluginAction(m_pShowSettingsAction);
}

//=============================================================================================================

BrainFlowBoard::~BrainFlowBoard()
{
    releaseSession(false);
}

//=============================================================================================================

void BrainFlowBoard::showSettings()
{
    BrainFlowStreamingWidget *widget = new BrainFlowStreamingWidget(this);
    widget->show();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> BrainFlowBoard::clone() const
{
    QSharedPointer<BrainFlowBoard> pClone(new BrainFlowBoard());
    return pClone;
}

//=============================================================================================================

void BrainFlowBoard::init()
{
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainFlowBoard", "BrainFlow Board Output");
    m_pOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pOutput);

    BoardShim::set_log_file((char *)"brainflow_log.txt");
    BoardShim::enable_dev_board_logger();
}

//=============================================================================================================

void BrainFlowBoard::unload()
{
}

//=============================================================================================================

void BrainFlowBoard::setUpFiffInfo()
{
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    m_pFiffInfo->nchan = (int)m_vChannels.size();
    m_pFiffInfo->sfreq = m_iSamplingFreq;
    m_pFiffInfo->highpass = 0.001f;
    m_pFiffInfo->lowpass = m_iSamplingFreq/2;

    //
    //Set up the channel info
    //
    QStringList QSLChNames;
    m_pFiffInfo->chs.clear();

    for(int i = 0; i < m_pFiffInfo->nchan; ++i)
    {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

        //Set channel name
        sChType = QString("Ch ");
        if(i<10) {
            sChType.append("00");
        }

        if(i>=10 && i<100) {
            sChType.append("0");
        }

        fChInfo.ch_name = sChType.append(sChType.number(i));

        //Set channel type
        fChInfo.kind = FIFFV_EEG_CH;

        //Set logno
        fChInfo.logNo = i;

        //Set coord frame
        fChInfo.coord_frame = FIFFV_COORD_HEAD;

        //Set unit
        fChInfo.unit = FIFF_UNIT_V;

        //Set EEG electrode location - Convert from mm to m
        fChInfo.eeg_loc(0,0) = 0;
        fChInfo.eeg_loc(1,0) = 0;
        fChInfo.eeg_loc(2,0) = 0;

        //Set EEG electrode direction - Convert from mm to m
        fChInfo.eeg_loc(0,1) = 0;
        fChInfo.eeg_loc(1,1) = 0;
        fChInfo.eeg_loc(2,1) = 0;

        //Also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
        fChInfo.chpos.r0(0) = 0;
        fChInfo.chpos.r0(1) = 0;
        fChInfo.chpos.r0(2) = 0;

        fChInfo.chpos.ex(0) = 1;
        fChInfo.chpos.ex(1) = 0;
        fChInfo.chpos.ex(2) = 0;

        fChInfo.chpos.ey(0) = 0;
        fChInfo.chpos.ey(1) = 1;
        fChInfo.chpos.ey(2) = 0;

        fChInfo.chpos.ez(0) = 0;
        fChInfo.chpos.ez(1) = 0;
        fChInfo.chpos.ez(2) = 1;

//        //Digital input channel
//        if(i == m_pFiffInfo->nchan-1)
//        {
//            //Set channel type
//            fChInfo.kind = FIFFV_STIM_CH;

//            sChType = QString("STIM");
//            fChInfo.ch_name = sChType;
//        }

        QSLChNames << sChType;

        m_pFiffInfo->chs.append(fChInfo);
    }

    //Set channel names in fiff_info_base
    m_pFiffInfo->ch_names = QSLChNames;

    //
    //Set head projection
    //
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}

//=============================================================================================================

bool BrainFlowBoard::start()
{
    if (!m_pBoardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Configure streaming session before run!");
        msgBox.exec();
        return false;
    }
    try {
        m_pBoardShim->start_stream(m_iSamplingFreq * 100, (char*)m_sStreamerParams.c_str());
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        QMessageBox msgBox;
        msgBox.setText("Failed to start streaming.");
        msgBox.exec();
        return false;
    }
    m_bProcessOutput = true;
    m_OutputProcessingThread = std::thread(&BrainFlowBoard::run, this);
    return true;
}

//=============================================================================================================

bool BrainFlowBoard::stop()
{
    try {
        if(m_pBoardShim) {
            m_pBoardShim->stop_stream();
        }
        m_bProcessOutput = false;

        if(m_OutputProcessingThread.joinable()){
            m_OutputProcessingThread.join();
        }
        m_pOutput->measurementData()->clear();
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        return false;
    }
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType BrainFlowBoard::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString BrainFlowBoard::getName() const
{
    return "BrainFlow";
}

//=============================================================================================================

QWidget* BrainFlowBoard::setupWidget()
{
    BrainFlowSetupWidget* widget = new BrainFlowSetupWidget(this);
    return widget;
}

//=============================================================================================================

void BrainFlowBoard::prepareSession(BrainFlowInputParams params,
                                    std::string streamerParams,
                                    int boardId,
                                    int dataType)
{
    if (m_pBoardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

    QMessageBox msgBox;
    m_iBoardId = boardId;
    m_sStreamerParams = streamerParams;

    try {
        switch (dataType)
        {
            case 0:
                m_vChannels = BoardShim::get_eeg_channels(boardId);
                break;
            case 1:
                m_vChannels = BoardShim::get_emg_channels(boardId);
                break;
            case 2:
                m_vChannels = BoardShim::get_ecg_channels(boardId);
                break;
            case 3:
                m_vChannels = BoardShim::get_eog_channels(boardId);
                break;
            case 4:
                m_vChannels = BoardShim::get_eda_channels(boardId);
                break;
            default:
                throw BrainFlowException ("unsupported data type", (int)BrainFlowExitCodes::UNSUPPORTED_BOARD_ERROR);
                break;
        }
        m_iSamplingFreq = BoardShim::get_sampling_rate(boardId);
        m_pBoardShim = new BoardShim(m_iBoardId, params);
        m_pBoardShim->prepare_session();

        setUpFiffInfo();

        m_pOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pOutput->measurementData()->setMultiArraySize(1);

        msgBox.setText("Streaming session is ready");
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        msgBox.setText("Invalid parameters. Check logs for details.");

        delete m_pBoardShim;
        m_pBoardShim = NULL;
    }

    msgBox.exec();
}

//=============================================================================================================

void BrainFlowBoard::configureBoard(std::string config)
{
    QMessageBox msgBox;
    if (!m_pBoardShim)
    {
        msgBox.setText("Prepare Session first.");
        return;
    }
    else
    {
        try {
            m_pBoardShim->config_board((char *)config.c_str());
            msgBox.setText("Configured.");
        } catch (const BrainFlowException &err) {
            BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
            msgBox.setText("Failed to Configure.");
        }
    }
    msgBox.exec();
}

//=============================================================================================================

void BrainFlowBoard::run()
{
    int iMinSamples = 10;
    unsigned long lSamplingPeriod = (unsigned long)(1000000.0 / m_iSamplingFreq) * iMinSamples;
    int numRows = BoardShim::get_num_rows(m_iBoardId);

    int iSampleIterator, i, j;
    iSampleIterator = 0;
    Eigen::VectorXd vec(m_vChannels.size());
    Eigen::MatrixXd matrix (m_vChannels.size(), m_uiSamplesPerBlock);
    BrainFlowArray<double, 2> data;
    QList<Eigen::VectorXd> lSampleBlockBuffer;

    while(m_bProcessOutput) {
        std::this_thread::sleep_for(std::chrono::microseconds(lSamplingPeriod));
        iSampleIterator = 0;

        //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
        while(iSampleIterator < m_uiSamplesPerBlock && m_bProcessOutput) {
            //Get sample block from device
            data = m_pBoardShim->get_board_data ();
            if (data.get_size(1) == 0) {
                continue;
            }

            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received.
            //These are then written to the next matrix (block)
            for(i = 0; i < data.get_size(1); ++i) {
                for(j = 0; j < m_vChannels.size(); ++j) {
                    vec(j) = data.at(m_vChannels[j], i)/1000000.0;
                }

                lSampleBlockBuffer.push_back(vec);
            }

            //Fill matrix with data from buffer
            while(!lSampleBlockBuffer.isEmpty()) {
                if(iSampleIterator >= m_uiSamplesPerBlock) {
                    break;
                }

                matrix.col(iSampleIterator) = lSampleBlockBuffer.takeFirst();

                iSampleIterator++;
            }
        }

        m_pOutput->measurementData()->setValue(matrix);
    }
}

//=============================================================================================================

void BrainFlowBoard::releaseSession(bool useQmessage)
{
    if (m_pBoardShim)
    {
        try {
            m_pBoardShim->release_session();
        } catch (...) {
        }
    }
    delete m_pBoardShim;

    m_pBoardShim = nullptr;
    m_iBoardId = (int)BoardIds::SYNTHETIC_BOARD;
    m_iSamplingFreq = 0;
    m_sStreamerParams = "";

    if (useQmessage)
    {
        QMessageBox msgBox;
        msgBox.setText("Released.");
        msgBox.exec();
    }
}

//=============================================================================================================

QString BrainFlowBoard::getBuildInfo()
{
    return QString(BRAINFLOWBOARDPLUGIN::buildDateTime()) + QString(" - ")  + QString(BRAINFLOWBOARDPLUGIN::buildHash());
}
