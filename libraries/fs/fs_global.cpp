#include "fs_global.h"

const char* FSLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FSLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FSLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
