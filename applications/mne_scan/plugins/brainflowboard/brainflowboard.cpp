#include <QMessageBox>

#include "data_filter.h"

#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"
#include "FormFiles/brainflowstreamingwidget.h"


BrainFlowBoard::BrainFlowBoard() :
    boardShim(NULL),
    channels(NULL),
    boardId((int)BoardIds::SYNTHETIC_BOARD),
    isRunning(false),
    output(NULL),
    samplingRate(0),
    streamerParams(""),
    numChannels(0),
    bandStart(1),
    bandStop(30),
    notchFreq(50),
    filterType((int)FilterTypes::BUTTERWORTH),
    filterOrder(4),
    ripple(0.5)
{
    showSettingsAction = new QAction(QIcon(":/images/options.png"), tr("Streaming Settings"),this);
    showSettingsAction->setStatusTip(tr("Streaming Settings"));
    connect(showSettingsAction, &QAction::triggered, this, &BrainFlowBoard::showSettings);
    addPluginAction(showSettingsAction);
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
    if (!boardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Configure streaming session before run!");
        msgBox.exec();
        return false;
    }
    try {
        boardShim->start_stream(samplingRate * 1000, (char*)streamerParams.c_str());
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        QMessageBox msgBox;
        msgBox.setText("Failed to start streaming.");
        msgBox.exec();
        return false;
    }
    isRunning = true;
    QThread::start();
    return true;
}

bool BrainFlowBoard::stop()
{
    try {
        boardShim->stop_stream();
        isRunning = false;
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
    if (boardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

    BoardShim::log_message((int)LogLevels::LEVEL_ERROR, "Vert Scale is %d %d", vertScale);
    QMessageBox msgBox;
    this->boardId = boardId;
    this->streamerParams = streamerParams;

    try {
        switch (dataType)
        {
            case 0:
                channels = BoardShim::get_eeg_channels(boardId, &numChannels);
                break;
            case 1:
                channels = BoardShim::get_emg_channels(boardId, &numChannels);
                break;
            case 2:
                channels = BoardShim::get_ecg_channels(boardId, &numChannels);
                break;
            case 3:
                channels = BoardShim::get_eog_channels(boardId, &numChannels);
                break;
            case 4:
                channels = BoardShim::get_eda_channels(boardId, &numChannels);
                break;
            default:
                throw BrainFlowException ("unsupported data type", UNSUPPORTED_BOARD_ERROR);
                break;
        }
        samplingRate = BoardShim::get_sampling_rate(boardId);
        boardShim = new BoardShim(boardId, params);
        boardShim->prepare_session();

        output = new PluginOutputData<RealTimeSampleArray>::SPtr[numChannels];
        for (int i = 0; i < numChannels; i++)
        {
            output[i] = PluginOutputData<RealTimeSampleArray>::create(this, "Channel", "Channel output data");
            m_outputConnectors.append(output[i]);
            output[i]->data()->setSamplingRate(samplingRate);
            output[i]->data()->setVisibility(true);
            output[i]->data()->setArraySize(1);
            output[i]->data()->setMinValue((double)(0 - vertScale));
            output[i]->data()->setMaxValue((double)vertScale);
            output[i]->data()->setUnit("uV");
        }

        msgBox.setText("Streaming session is ready");
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        msgBox.setText("Invalid parameters. Check logs for details.");

        delete[] channels;
        delete boardShim;
        channels = NULL;
        boardShim = NULL;
        numChannels = 0;
        delete[] output;
    }

    msgBox.exec();
}

void BrainFlowBoard::configureBoard(std::string config)
{
    QMessageBox msgBox;
    if (!boardShim)
    {
        msgBox.setText("Prepare Session first.");
        return;
    }
    else
    {
        try {
            boardShim->config_board((char *)config.c_str());
            msgBox.setText("Configured.");
        } catch (const BrainFlowException &err) {
            BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
            msgBox.setText("Failed to Configure.");
        }
    }
    msgBox.exec();
}

void BrainFlowBoard::applyFilters(int notchFreq, int bandStart, int bandStop, int filterType, int filterOrder, double ripple)
{
    this->notchFreq = notchFreq;
    this->bandStart = bandStart;
    this->bandStop = bandStop;
    this->filterType = filterType;
    this->filterOrder = filterOrder;
    this->ripple = ripple;
}

void BrainFlowBoard::run()
{
    unsigned long samplingPeriod = (unsigned long)(1000000.0 / samplingRate);
    int numRows = BoardShim::get_num_rows(boardId);

    double **data = NULL;
    int dataCount = 0;
    while(isRunning)
    {
        usleep(samplingPeriod);
        data = boardShim->get_board_data (&dataCount);

        for (int j = 0; j < numChannels; j++)
        {
            if (notchFreq != 0)
            {
                // notch filter is bandstop filter
                DataFilter::perform_bandstop(data[channels[j]], dataCount, samplingRate, (double)notchFreq, 2.0, filterOrder, filterType, ripple);
            }
            if ((bandStop != 0) && (bandStop != 0))
            {
                double centerFreq = bandStart + (double)(bandStop - bandStart) / 2.0;
                double width = (double)(bandStop - bandStart) / 2.0;
                DataFilter::perform_bandpass(data[channels[j]], dataCount, samplingRate,  centerFreq, width, filterOrder, filterType, ripple);
            }
            for (int i = 0; i < dataCount; i++)
            {
                output[j]->data()->setValue(data[channels[j]][i]);
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
    if (boardShim)
    {
        try {
            boardShim->release_session();
        } catch (...) {
        }
    }
    delete boardShim;
    delete[] channels;
    delete[] output;

    for (int i = 0; i < numChannels; i++)
    {
        m_outputConnectors.pop_back();
    }
    boardShim = NULL;
    channels = NULL;
    boardId = (int)BoardIds::SYNTHETIC_BOARD;
    isRunning = false;
    output = NULL;
    samplingRate = 0;
    streamerParams = "";
    numChannels = 0;

    if (useQmessage)
    {
        QMessageBox msgBox;
        msgBox.setText("Released.");
        msgBox.exec();
    }
}
