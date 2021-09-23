#include "connectivity_global.h"

const char* CONNECTIVITYLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* CONNECTIVITYLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* CONNECTIVITYLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
