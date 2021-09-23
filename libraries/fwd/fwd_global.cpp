#include "fwd_global.h"

const char* FWDLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FWDLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* FWDLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
