#include "dataloader_global.h"

const char* DATALOADERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* DATALOADERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* DATALOADERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
