#include "events_global.h"

const char* EVENTSPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* EVENTSPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* EVENTSPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};

