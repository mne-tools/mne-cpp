#include "rtcmne_global.h"

const char* RTCMNEPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* RTCMNEPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* RTCMNEPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
