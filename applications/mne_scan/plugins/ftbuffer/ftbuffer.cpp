
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace FTBUFFERPLUGIN;

FtBuffer::FtBuffer() {}

FtBuffer::~FtBuffer() {}


QSharedPointer<IPlugin> FtBuffer::clone() const {
    QSharedPointer<FtBuffer> pointer_FtBuffer(new FtBuffer);
    return pointer_FtBuffer;
}

void FtBuffer::init() {}

void FtBuffer::unload() {}

bool FtBuffer::start() {
    return true;
}

bool FtBuffer::stop() {
    return true;
}

IPlugin::PluginType FtBuffer::getType() const {
    return _ISensor;
}

QString FtBuffer::getName() const {
    return "FtBuffer";
}

inline bool FtBuffer::multiInstanceAllowed() const {
    return true;
}

QWidget* FtBuffer::setupWidget() {
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this);
    return setupWidget;
}

void FtBuffer::run() {}
