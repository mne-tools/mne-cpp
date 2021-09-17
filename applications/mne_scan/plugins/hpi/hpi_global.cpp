#include "hpi_global.h"

const char* HPIPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* HPIPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* HPIPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
