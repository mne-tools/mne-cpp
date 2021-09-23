#include "rtcmne_global.h"

const char* RTCMNEPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* RTCMNEPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* RTCMNEPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
