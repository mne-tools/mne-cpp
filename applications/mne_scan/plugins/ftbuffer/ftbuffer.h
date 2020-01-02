#ifndef FTBUFFER_H
#define FTBUFFER_H

#include "ftbuffer_global.h"

#include <scShared/Interfaces/ISensor.h>
#include <scShared/Interfaces/IAlgorithm.h>

class FTBUFFER_EXPORT FtBuffer : public SCSHAREDLIB::ISensor
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ftbuffer.json")

    Q_INTERFACES(SCSHAREDLIB::ISensor)

public:

    FtBuffer();

    ~FtBuffer();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initializes the plugin.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns the plugin type.
    * Pure virtual method inherited by IModule.
    *
    * @return type of the ISensor
    */
    virtual PluginType getType() const;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the ISensor.
    */
    virtual QString getName() const;

    //=========================================================================================================
    /**
    * True if multi instantiation of plugin is allowed.
    *
    * @return true if multi instantiation of plugin is allowed.
    */
    virtual inline bool multiInstanceAllowed() const;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget();


protected:
    virtual void run();


};

#endif // FTBUFFER_H
