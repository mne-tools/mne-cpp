#include "tmsi_global.h"

const char* TMSIPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* TMSIPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* TMSIPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
