#include "events_global.h"

const char* EVENTSLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* EVENTSLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* EVENTSLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
