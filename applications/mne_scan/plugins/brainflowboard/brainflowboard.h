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

    std::string streamerParams;
    int boardId;
    BoardShim *boardShim;
    int numChannels;
    int *channels;
    int samplingRate;
    PluginOutputData<RealTimeSampleArray>::SPtr *output;
    volatile int notchFreq;
    volatile int bandStart;
    volatile int bandStop;
    volatile int filterOrder;
    volatile int filterType;
    volatile double ripple;
    volatile bool isRunning;

    QAction *showSettingsAction;
};

#endif // BRAINFLOWBOARD_H
