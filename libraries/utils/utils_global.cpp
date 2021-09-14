#include "utils_global.h"

const char* UTILSLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* UTILSLIB::BUILD_DATE(){ return BUILDINFO::date();};

const char* UTILSLIB::BUILD_HASH(){ return BUILDINFO::githash();};

const char* UTILSLIB::BUILD_HASH_LONG(){ return BUILDINFO::githash_long();};
