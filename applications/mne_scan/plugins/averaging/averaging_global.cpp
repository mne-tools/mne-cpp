#include "averaging_global.h"

const char* AVERAGINGPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* AVERAGINGPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* AVERAGINGPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
