#include "fiff_global.h"

const char* FIFFLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FIFFLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* FIFFLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
