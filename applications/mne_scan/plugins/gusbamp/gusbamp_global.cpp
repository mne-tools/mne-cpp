#include "gusbamp_global.h"

const char* GUSBAMPPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* GUSBAMPPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* GUSBAMPPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
