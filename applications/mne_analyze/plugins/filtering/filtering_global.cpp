#include "filtering_global.h"

const char* FILTERINGPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FILTERINGPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FILTERINGPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
