#include "babymeg_global.h"

const char* BABYMEGPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* BABYMEGPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* BABYMEGPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
