#include "writetofile_global.h"

const char* WRITETOFILEPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* WRITETOFILEPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* WRITETOFILEPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
