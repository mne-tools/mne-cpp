#include "natus_global.h"

const char* NATUSPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* NATUSPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* NATUSPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
