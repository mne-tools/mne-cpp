#include "rtfwd_global.h"

const char* RTFWDPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* RTFWDPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* RTFWDPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
