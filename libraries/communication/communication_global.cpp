#include "communication_global.h"

const char* COMMUNICATIONLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* COMMUNICATIONLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* COMMUNICATIONLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
