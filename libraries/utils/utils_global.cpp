#include "utils_global.h"

const char* UTILSLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* UTILSLIB::BUILD_DATE(){ return BUILDINFO::date();};
