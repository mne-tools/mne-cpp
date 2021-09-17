#include "natus_global.h"

const char* NATUSPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* NATUSPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* NATUSPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
