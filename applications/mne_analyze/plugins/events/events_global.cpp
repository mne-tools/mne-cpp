#include "events_global.h"

const char* EVENTSPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* EVENTSPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* EVENTSPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};

