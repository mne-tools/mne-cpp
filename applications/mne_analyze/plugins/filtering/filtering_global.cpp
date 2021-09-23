#include "filtering_global.h"

const char* FILTERINGPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FILTERINGPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* FILTERINGPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
