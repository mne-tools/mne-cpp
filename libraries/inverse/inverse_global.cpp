#include "inverse_global.h"

const char* INVERSELIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* INVERSELIB::buildHash(){ return UTILSLIB::gitHash();};

const char* INVERSELIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
