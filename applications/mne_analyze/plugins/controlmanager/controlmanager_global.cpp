#include "controlmanager_global.h"

const char* CONTROLMANAGERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* CONTROLMANAGERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* CONTROLMANAGERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
