#include "hpi_global.h"

const char* HPIPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* HPIPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* HPIPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
