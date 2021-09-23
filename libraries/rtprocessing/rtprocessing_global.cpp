#include "rtprocessing_global.h"

const char* RTPROCESSINGLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* RTPROCESSINGLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* RTPROCESSINGLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};

