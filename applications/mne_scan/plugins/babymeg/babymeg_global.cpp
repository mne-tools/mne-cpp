#include "babymeg_global.h"

const char* BABYMEGPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* BABYMEGPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* BABYMEGPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
