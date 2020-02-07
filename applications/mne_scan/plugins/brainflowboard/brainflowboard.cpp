#include <QMessageBox>

#include "data_filter.h"

#include <scMeas/realtimemultisamplearray.h>

#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"
#include "FormFiles/brainflowstreamingwidget.h"


BrainFlowBoard::BrainFlowBoard() :
    m_pBoardShim(NULL),
    m_pChannels(NULL),
    m_iBoardId((int)BoardIds::SYNTHETIC_BOARD),
    m_bIsRunning(false),
    m_pOutput(NULL),
    m_iSamplingRate(0),
    m_sStreamerParams(""),
    m_iNumChannels(0)
{
    m_pShowSettingsAction = new QAction(QIcon(":/images/options.png"), tr("Streaming Settings"),this);
    m_pShowSettingsAction->setStatusTip(tr("Streaming Settings"));
    connect(m_pShowSettingsAction, &QAction::triggered, this, &BrainFlowBoard::showSettings);
    addPluginAction(m_pShowSettingsAction);
    m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo> (new FIFFLIB::FiffInfo);
}

BrainFlowBoard::~BrainFlowBoard()
{
    releaseSession(false);
}

void BrainFlowBoard::showSettings()
{
    BrainFlowStreamingWidget *widget = new BrainFlowStreamingWidget(this);
    widget->show();
}

QSharedPointer<IPlugin> BrainFlowBoard::clone() const
{
    QSharedPointer<BrainFlowBoard> pClone(new BrainFlowBoard());
    return pClone;
}

void BrainFlowBoard::init()
{
    BoardShim::set_log_file((char *)"brainflow_log.txt");
    BoardShim::enable_dev_board_logger();
}

void BrainFlowBoard::unload()
{
}

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
        m_pBoardShim->start_stream(m_iSamplingRate * 100, (char*)m_sStreamerParams.c_str());
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

IPlugin::PluginType BrainFlowBoard::getType() const
{
    return _ISensor;
}

QString BrainFlowBoard::getName() const
{
    return "BrainFlow Board Plugin";
}

QWidget* BrainFlowBoard::setupWidget()
{
    BrainFlowSetupWidget* widget = new BrainFlowSetupWidget(this);
    return widget;
}

void BrainFlowBoard::prepareSession(BrainFlowInputParams params, std::string streamerParams, int boardId, int dataType, int vertScale)
{
    if (m_pBoardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

    BoardShim::log_message((int)LogLevels::LEVEL_ERROR, "Vert Scale is %d %d", vertScale);
    QMessageBox msgBox;
    m_iBoardId = boardId;
    m_sStreamerParams = streamerParams;

    try {
        switch (dataType)
        {
            case 0:
                m_pChannels = BoardShim::get_eeg_channels(boardId, &m_iNumChannels);
                break;
            case 1:
                m_pChannels = BoardShim::get_emg_channels(boardId, &m_iNumChannels);
                break;
            case 2:
                m_pChannels = BoardShim::get_ecg_channels(boardId, &m_iNumChannels);
                break;
            case 3:
                m_pChannels = BoardShim::get_eog_channels(boardId, &m_iNumChannels);
                break;
            case 4:
                m_pChannels = BoardShim::get_eda_channels(boardId, &m_iNumChannels);
                break;
            default:
                throw BrainFlowException ("unsupported data type", UNSUPPORTED_BOARD_ERROR);
                break;
        }
        m_iSamplingRate = BoardShim::get_sampling_rate(boardId);
        m_pBoardShim = new BoardShim(m_iBoardId, params);
        m_pBoardShim->prepare_session();

        m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainFlowBoard", "BrainFlow Board Output");
        m_pFiffInfo->nchan = m_iNumChannels;
        m_pFiffInfo->sfreq = m_iSamplingRate;
        m_pOutput->data()->initFromFiffInfo(m_pFiffInfo);
        m_pOutput->data()->setVisibility(true);
        m_pOutput->data()->setMultiArraySize(1);
        m_outputConnectors.append(m_pOutput);

        msgBox.setText("Streaming session is ready");
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        msgBox.setText("Invalid parameters. Check logs for details.");

        delete[] m_pChannels;
        delete m_pBoardShim;
        m_pChannels = NULL;
        m_pBoardShim = NULL;
        m_iNumChannels = 0;
    }

    msgBox.exec();
}

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

void BrainFlowBoard::run()
{
    unsigned long samplingPeriod = (unsigned long)(1000000.0 / m_iSamplingRate);
    int numRows = BoardShim::get_num_rows(m_iBoardId);

    double **data = NULL;
    int dataCount = 0;
    while(m_bIsRunning)
    {
        usleep(samplingPeriod);
        data = m_pBoardShim->get_board_data (&dataCount);
        if (dataCount == 0)
        {
            continue;
        }

        for (int j = 0; j < m_iNumChannels; j++)
        {
            for (int i = 0; i < dataCount; i++)
            {
                m_pOutput[j]->data()->setValue(data[m_pChannels[j]][i]);
            }
        }

        for (int i = 0; i < numRows; i++)
        {
            delete[] data[i];
        }
        delete[] data;
    }
}

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
    delete[] m_pChannels;

    for (int i = 0; i < m_iNumChannels; i++)
    {
        m_outputConnectors.pop_back();
    }
    m_pBoardShim = NULL;
    m_pChannels = NULL;
    m_iBoardId = (int)BoardIds::SYNTHETIC_BOARD;
    m_bIsRunning = false;
    m_pOutput = NULL;
    m_iSamplingRate = 0;
    m_sStreamerParams = "";
    m_iNumChannels = 0;
    m_pFiffInfo->clear();

    if (useQmessage)
    {
        QMessageBox msgBox;
        msgBox.setText("Released.");
        msgBox.exec();
    }
}
