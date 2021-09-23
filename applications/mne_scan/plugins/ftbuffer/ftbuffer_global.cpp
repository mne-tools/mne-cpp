#include "ftbuffer_global.h"

const char* FTBUFFERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FTBUFFERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* FTBUFFERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};


