#include "coregistration_global.h"

const char* COREGISTRATIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* COREGISTRATIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* COREGISTRATIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
