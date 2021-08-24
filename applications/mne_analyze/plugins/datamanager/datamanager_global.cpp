#include "datamanager_global.h"

const char* DATAMANAGERPLUGIN::BUILD_TIME(){ return BUILDINFO::time();};

const char* DATAMANAGERPLUGIN::BUILD_DATE(){ return BUILDINFO::date();};
