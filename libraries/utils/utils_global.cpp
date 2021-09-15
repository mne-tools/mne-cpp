#include "utils_global.h"

const char* UTILSLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* UTILSLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* UTILSLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
