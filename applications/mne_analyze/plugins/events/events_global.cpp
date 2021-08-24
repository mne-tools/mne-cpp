#include "events_global.h"

const char* EVENTSPLUGIN::BUILD_TIME(){ return BUILDINFO::time();};

const char* EVENTSPLUGIN::BUILD_DATE(){ return BUILDINFO::date();};
