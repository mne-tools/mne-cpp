#include "datamanager_global.h"

const char* DATAMANAGERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* DATAMANAGERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* DATAMANAGERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
