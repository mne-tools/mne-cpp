#include "tmsi_global.h"

const char* TMSIPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* TMSIPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* TMSIPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
