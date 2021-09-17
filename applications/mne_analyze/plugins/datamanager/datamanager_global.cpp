#include "datamanager_global.h"

const char* DATAMANAGERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* DATAMANAGERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* DATAMANAGERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
