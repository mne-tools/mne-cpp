#include "controlmanager_global.h"

const char* CONTROLMANAGERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* CONTROLMANAGERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* CONTROLMANAGERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
