#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

#include "brainflowboard_global.h"

#include <scShared/Interfaces/ISensor.h>
#include <scMeas/realtimesamplearray.h>

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
    void applyFilters(int notchFreq, int bandStart, int bandStop, int filterType, int filterOrder, double ripple);
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
    PluginOutputData<RealTimeSampleArray>::SPtr *m_pOutput;
    volatile int m_iNotchFreq;
    volatile int m_iBandStart;
    volatile int m_iBandStop;
    volatile int m_iFilterOrder;
    volatile int m_iFilterType;
    volatile double m_dRipple;
    volatile bool m_bIsRunning;

    QAction *m_pShowSettingsAction;
};

#endif // BRAINFLOWBOARD_H
