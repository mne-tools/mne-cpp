#include "view3d_global.h"

const char* VIEW3DPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* VIEW3DPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* VIEW3DPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};

