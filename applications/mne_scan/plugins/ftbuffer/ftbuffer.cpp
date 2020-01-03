#include "ftbuffer.h"

using namespace SCSHAREDLIB;

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
    QWidget* setupWidget = new FtBufferGUI;
    return setupWidget;
}

void FtBuffer::run() {}
