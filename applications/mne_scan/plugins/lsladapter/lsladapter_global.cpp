#include "lsladapter_global.h"

const char* LSLADAPTERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* LSLADAPTERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* LSLADAPTERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
