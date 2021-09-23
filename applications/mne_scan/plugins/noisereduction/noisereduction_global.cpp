#include "noisereduction_global.h"

const char* NOISEREDUCTIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* NOISEREDUCTIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* NOISEREDUCTIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
