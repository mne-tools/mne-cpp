#include "channelselection_global.h"

const char* CHANNELSELECTIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* CHANNELSELECTIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* CHANNELSELECTIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
