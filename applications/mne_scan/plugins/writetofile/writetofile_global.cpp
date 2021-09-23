#include "writetofile_global.h"

const char* WRITETOFILEPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* WRITETOFILEPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* WRITETOFILEPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
