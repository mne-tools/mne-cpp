#include "rtcmne_global.h"

const char* RTCMNEPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* RTCMNEPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* RTCMNEPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
