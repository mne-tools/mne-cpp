#include "fiff_global.h"

const char* FIFFLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FIFFLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FIFFLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
