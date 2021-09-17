#include "communication_global.h"

const char* COMMUNICATIONLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* COMMUNICATIONLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* COMMUNICATIONLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
