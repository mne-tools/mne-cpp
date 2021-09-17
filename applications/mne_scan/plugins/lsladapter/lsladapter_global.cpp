#include "lsladapter_global.h"

const char* LSLADAPTERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* LSLADAPTERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* LSLADAPTERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
