#include "fs_global.h"

const char* FSLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* FSLIB::BUILD_DATE(){ return BUILDINFO::date();};
