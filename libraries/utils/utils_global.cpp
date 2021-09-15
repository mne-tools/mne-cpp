#include "utils_global.h"

const char* UTILSLIB::BUILD_DATETIME(){ return BUILDINFO::timestamp();};

const char* UTILSLIB::BUILD_HASH(){ return BUILDINFO::githash();};

const char* UTILSLIB::BUILD_HASH_LONG(){ return BUILDINFO::githash_long();};
