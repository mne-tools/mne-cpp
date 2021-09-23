#include "averaging_global.h"

const char* AVERAGINGPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* AVERAGINGPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* AVERAGINGPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
