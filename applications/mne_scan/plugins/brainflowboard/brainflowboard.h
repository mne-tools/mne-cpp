#ifndef BRAINFLOWBOARD_H
#define BRAINFLOWBOARD_H

#include "brainflowboard_global.h"

#include <scShared/Interfaces/ISensor.h>
#include <scMeas/realtimesamplearray.h>

#include "board_shim.h"

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;

class BrainFlowSetupWidget;


class BRAINFLOWBOARD_EXPORT BrainFlowBoard : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "brainflowboard.json")
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class BrainFlowSetupWidget;

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

protected:
    virtual void run();

private:
    void releaseSession(bool useQmessage = true);
    void prepareSession(BrainFlowInputParams params, std::string streamerParams, int boardId, int dataType);

    std::string streamerParams;
    int boardId;
    BoardShim *boardShim;
    int numChannels;
    int *channels;
    int samplingRate;
    bool isRunning;
    PluginOutputData<RealTimeSampleArray>::SPtr *output;
};

#endif // BRAINFLOWBOARD_H
