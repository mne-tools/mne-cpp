#include "writetofile_global.h"

const char* WRITETOFILEPLUGIN::BUILD_TIME(){ return BUILDINFO::time();};

const char* WRITETOFILEPLUGIN::BUILD_DATE(){ return BUILDINFO::date();};
