#include "dipolefit_global.h"

const char* DIPOLEFITPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* DIPOLEFITPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* DIPOLEFITPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
