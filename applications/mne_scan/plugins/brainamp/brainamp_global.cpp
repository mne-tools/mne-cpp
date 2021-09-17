#include "brainamp_global.h"

const char* BRAINAMPPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* BRAINAMPPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* BRAINAMPPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
