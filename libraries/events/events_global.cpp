#include "events_global.h"

const char* EVENTSLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* EVENTSLIB::BUILD_DATE(){ return BUILDINFO::date();};
