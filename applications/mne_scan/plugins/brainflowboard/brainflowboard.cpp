#include <QMessageBox>

#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"

BrainFlowBoard::BrainFlowBoard() :
    boardShim(NULL),
    channels(NULL),
    boardId((int)BoardIds::SYNTHETIC_BOARD),
    isRunning(false),
    output(NULL),
    samplingRate(0),
    streamerParams(""),
    numChannels(0),
    minValue(0),
    maxValue(0)
{

}

BrainFlowBoard::~BrainFlowBoard()
{
    releaseSession(false);
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

void BrainFlowBoard::prepareSession(BrainFlowInputParams params, std::string streamerParams, int boardId, int dataType, double maxValue, double minValue)
{
    if (boardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

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
                channels = BoardShim::get_ppg_channels(boardId, &numChannels);
                break;
            case 5:
                channels = BoardShim::get_eda_channels(boardId, &numChannels);
                break;
            default:
                throw BrainFlowException ("unsupported data type", UNSUPPORTED_BOARD_ERROR);
                break;
        }
        samplingRate = BoardShim::get_sampling_rate(boardId);
        boardShim = new BoardShim(boardId, params);
        boardShim->prepare_session();

        // if its not provided run streaming for a few seconds, get data and use this data as a reference for axis
        if ((abs(maxValue) < DBL_EPSILON) || (abs(minValue) < DBL_EPSILON))
        {
            boardShim->start_stream();
            sleep(2);
            boardShim->stop_stream();
            int dataCount = 0;
            double **data = boardShim->get_board_data(&dataCount);
            int numRows = BoardShim::get_num_rows(boardId);
            for (int i = 0; i < dataCount; i++)
            {
                for (int j = 0; j < numChannels; j++)
                {

                   if (abs(minValue) < DBL_EPSILON || (data[channels[j]][i] < minValue))
                   {
                       minValue = (int)data[channels[j]][i];
                   }
                   if (abs(maxValue) < DBL_EPSILON || (data[channels[j]][i] > maxValue))
                   {
                       maxValue = (int)data[channels[j]][i];
                   }
                }
            }
            // increare ref values by 1.5 times and use as axis
            maxValue = maxValue + (int)abs(maxValue) * 0.5;
            minValue = minValue - (int)abs(minValue) * 0.5;
            BoardShim::log_message((int)LogLevels::LEVEL_ERROR, "set maxValue to %lf and minValue to %lf", maxValue, minValue);
            for (int i = 0; i < numRows; i++)
            {
                delete[] data[i];
            }
            delete[] data;
        }

        output = new PluginOutputData<RealTimeSampleArray>::SPtr[numChannels];
        for (int i = 0; i < numChannels; i++)
        {
            output[i] = PluginOutputData<RealTimeSampleArray>::create(this, "Channel", "Channel output data");
            m_outputConnectors.append(output[i]);
            output[i]->data()->setSamplingRate(samplingRate);
            output[i]->data()->setVisibility(true);
            output[i]->data()->setArraySize(1);
            output[i]->data()->setMinValue(minValue);
            output[i]->data()->setMaxValue(maxValue);
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

void BrainFlowBoard::run()
{
    int samplingPeriod = 1000000.0 / samplingRate;
    int numRows = BoardShim::get_num_rows(boardId);

    double **data = NULL;
    int dataCount = 0;
    bool changeAxis = false;
    while(isRunning)
    {
        usleep(samplingPeriod);
        data = boardShim->get_board_data (&dataCount);

        for (int i = 0; i < dataCount; i++)
        {
            for (int j = 0; j < numChannels; j++)
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
