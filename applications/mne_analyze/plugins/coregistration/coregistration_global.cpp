#include "coregistration_global.h"

const char* COREGISTRATIONPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* COREGISTRATIONPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* COREGISTRATIONPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
