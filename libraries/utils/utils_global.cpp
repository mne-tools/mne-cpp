#include "utils_global.h"

const char* UTILSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* UTILSLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* UTILSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
