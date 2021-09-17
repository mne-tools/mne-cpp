#include "events_global.h"

const char* EVENTSLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* EVENTSLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* EVENTSLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
