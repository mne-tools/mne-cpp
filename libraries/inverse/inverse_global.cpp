#include "inverse_global.h"

const char* INVERSELIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* INVERSELIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* INVERSELIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
