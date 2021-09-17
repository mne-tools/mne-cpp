#include "connectivity_global.h"

const char* CONNECTIVITYLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* CONNECTIVITYLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* CONNECTIVITYLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
