#include "rtfwd_global.h"

const char* RTFWDPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* RTFWDPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* RTFWDPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
