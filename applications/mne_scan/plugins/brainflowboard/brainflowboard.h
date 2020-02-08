#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

#include "brainflowboard_global.h"

#include <scShared/Interfaces/ISensor.h>

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

#include "board_shim.h"

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


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

    void releaseSession(bool useQmessage = true);
    void prepareSession(BrainFlowInputParams params, std::string streamerParams, int boardId, int dataType, int vertScale);
    void configureBoard(std::string config);
    void showSettings();

protected:
    virtual void run();

private:

    std::string m_sStreamerParams;
    int m_iBoardId;
    BoardShim *m_pBoardShim;
    int m_iNumChannels;
    int *m_pChannels;
    int m_iSamplingRate;
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pOutput;
    volatile bool m_bIsRunning;

    QAction *m_pShowSettingsAction;
};

#endif // BRAINFLOWBOARD_H
