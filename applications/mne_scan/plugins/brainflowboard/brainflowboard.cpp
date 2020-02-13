//=============================================================================================================
/**
 * @file     brainflowboard.cpp
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
 * @brief Contains the definition of BrainFlowBoard class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include "data_filter.h"
#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"
#include "FormFiles/brainflowstreamingwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowBoard::BrainFlowBoard() :
    m_pBoardShim(nullptr),
    m_pChannels(nullptr),
    m_iBoardId((int)BoardIds::SYNTHETIC_BOARD),
    m_bIsRunning(false),
    m_pOutput(PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainFlowBoard", "BrainFlow Board Output")),
    m_iSamplingRate(0),
    m_sStreamerParams(""),
    m_iNumChannels(0),
    m_pFiffInfo(QSharedPointer<FiffInfo>::create())
{
    m_pShowSettingsAction = new QAction(QIcon(":/images/options.png"), tr("Streaming Settings"),this);
    m_pShowSettingsAction->setStatusTip(tr("Streaming Settings"));
    connect(m_pShowSettingsAction, &QAction::triggered, this, &BrainFlowBoard::showSettings);
    addPluginAction(m_pShowSettingsAction);
}

//*************************************************************************************************************

BrainFlowBoard::~BrainFlowBoard()
{
    releaseSession(false);
}

//*************************************************************************************************************

void BrainFlowBoard::showSettings()
{
    BrainFlowStreamingWidget *widget = new BrainFlowStreamingWidget(this);
    widget->show();
}

//*************************************************************************************************************

QSharedPointer<IPlugin> BrainFlowBoard::clone() const
{
    QSharedPointer<BrainFlowBoard> pClone(new BrainFlowBoard());
    return pClone;
}

//*************************************************************************************************************

void BrainFlowBoard::init()
{
    BoardShim::set_log_file((char *)"brainflow_log.txt");
    BoardShim::enable_dev_board_logger();
    m_outputConnectors.append(m_pOutput);
}

//*************************************************************************************************************

void BrainFlowBoard::unload()
{
}

//*************************************************************************************************************

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
        m_pBoardShim->start_stream(m_iSamplingRate * 10, (char*)m_sStreamerParams.c_str());
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        QMessageBox msgBox;
        msgBox.setText("Failed to start streaming.");
        msgBox.exec();
        return false;
    }
    m_bIsRunning = true;
    QThread::start();
    return true;
}

//*************************************************************************************************************

bool BrainFlowBoard::stop()
{
    try {
        m_pBoardShim->stop_stream();
        m_bIsRunning = false;
        m_pOutput->data()->clear();
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        return false;
    }
    return true;
}

//*************************************************************************************************************

IPlugin::PluginType BrainFlowBoard::getType() const
{
    return _ISensor;
}

//*************************************************************************************************************

QString BrainFlowBoard::getName() const
{
    return "BrainFlow Board Plugin";
}

//*************************************************************************************************************

QWidget* BrainFlowBoard::setupWidget()
{
    BrainFlowSetupWidget* widget = new BrainFlowSetupWidget(this);
    return widget;
}

//*************************************************************************************************************

void BrainFlowBoard::prepareSession(BrainFlowInputParams params, std::string sStreamerParams, int iBoardId, int iDataType)
{
    if (m_pBoardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

    QMessageBox msgBox;
    m_iBoardId = iBoardId;
    m_sStreamerParams = sStreamerParams;

    try {
        switch (iDataType)
        {
            case 0:
                m_pChannels = BoardShim::get_eeg_channels(iBoardId, &m_iNumChannels);
                break;
            case 1:
                m_pChannels = BoardShim::get_emg_channels(iBoardId, &m_iNumChannels);
                break;
            case 2:
                m_pChannels = BoardShim::get_ecg_channels(iBoardId, &m_iNumChannels);
                break;
            case 3:
                m_pChannels = BoardShim::get_eog_channels(iBoardId, &m_iNumChannels);
                break;
            case 4:
                m_pChannels = BoardShim::get_eda_channels(iBoardId, &m_iNumChannels);
                break;
            default:
                throw BrainFlowException ("unsupported data type", UNSUPPORTED_BOARD_ERROR);
                break;
        }
        m_iSamplingRate = BoardShim::get_sampling_rate(iBoardId);
        m_pBoardShim = QSharedPointer<BoardShim>( new BoardShim(m_iBoardId, params));
        m_pBoardShim->prepare_session();

        m_pFiffInfo->sfreq = m_iSamplingRate;
        m_pFiffInfo->nchan = m_iNumChannels;
        m_pFiffInfo->chs.clear();
        QStringList QSLChNames;
        for (int i = 0; i < m_pFiffInfo->nchan; i++)
        {
            FiffChInfo fChInfo;
            QString sChType = "Channel" + QString::number(i);
            fChInfo.ch_name = sChType;
            fChInfo.unit = FIFF_UNIT_V;
            fChInfo.unit_mul = 0;
            fChInfo.logNo = i;
            fChInfo.kind = FIFFV_EEG_CH;
            m_pFiffInfo->chs.append(fChInfo);
            QSLChNames << sChType;
        }
        m_pFiffInfo->ch_names = QSLChNames;
        m_pOutput->data()->clear();
        m_pOutput->data()->initFromFiffInfo(m_pFiffInfo);
        m_pOutput->data()->setMultiArraySize(1);
        m_pOutput->data()->setVisibility(true);
        msgBox.setText("Streaming session is ready");
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        msgBox.setText("Invalid parameters. Check logs for details.");
        delete[] m_pChannels;
        m_pChannels = nullptr;
        m_pBoardShim = nullptr;
        m_iNumChannels = 0;
    }

    msgBox.exec();
}

//*************************************************************************************************************

void BrainFlowBoard::configureBoard(std::string sConfig)
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
            m_pBoardShim->config_board((char *)sConfig.c_str());
            msgBox.setText("Configured.");
        } catch (const BrainFlowException &err) {
            BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
            msgBox.setText("Failed to Configure.");
        }
    }
    msgBox.exec();
}

//*************************************************************************************************************

void BrainFlowBoard::run()
{
    int iMinSamples = 10;
    unsigned long lSamplingPeriod = (unsigned long)(1000000.0 / m_iSamplingRate) * iMinSamples;
    int iNumRows = BoardShim::get_num_rows(m_iBoardId);

    double **pData = nullptr;
    int iDataCount = 0;
    while(m_bIsRunning)
    {
        usleep(lSamplingPeriod);
        pData = m_pBoardShim->get_board_data (&iDataCount);
        if (iDataCount == 0)
        {
            continue;
        }

        Eigen::MatrixXd matrix (m_iNumChannels, iDataCount);
        for (int j = 0; j < m_iNumChannels; j++)
        {
            for (int i = 0; i < iDataCount; i++)
            {
                matrix(j, i) = pData[m_pChannels[j]][i] / 1000000.0; // from uV to V
            }
        }
        m_pOutput->data()->setValue(matrix);

        for (int i = 0; i < iNumRows; i++)
        {
            delete[] pData[i];
        }
        delete[] pData;
    }
}

//*************************************************************************************************************

void BrainFlowBoard::releaseSession(bool bUseQmessage)
{
    if (m_pBoardShim)
    {
        try {
            m_pBoardShim->release_session();
        } catch (...) {
        }
    }
    delete[] m_pChannels;
    m_pBoardShim = nullptr;
    m_pChannels = nullptr;
    m_iBoardId = (int)BoardIds::SYNTHETIC_BOARD;
    m_bIsRunning = false;
    m_pOutput->data()->clear();
    m_iSamplingRate = 0;
    m_sStreamerParams = "";
    m_iNumChannels = 0;
    m_pFiffInfo->clear();

    if (bUseQmessage)
    {
        QMessageBox msgBox;
        msgBox.setText("Released.");
        msgBox.exec();
    }
}
