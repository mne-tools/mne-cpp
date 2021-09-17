#include "ftbuffer_global.h"

const char* FTBUFFERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FTBUFFERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FTBUFFERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};


