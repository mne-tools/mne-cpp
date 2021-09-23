#include "fs_global.h"

const char* FSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FSLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* FSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
