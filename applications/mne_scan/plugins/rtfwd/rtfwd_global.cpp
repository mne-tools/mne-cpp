#include "rtfwd_global.h"

const char* RTFWDPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* RTFWDPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* RTFWDPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
