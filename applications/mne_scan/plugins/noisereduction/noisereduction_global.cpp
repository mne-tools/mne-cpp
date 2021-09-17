#include "noisereduction_global.h"

const char* NOISEREDUCTIONPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* NOISEREDUCTIONPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* NOISEREDUCTIONPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
