#include "natus_global.h"

const char* NATUSPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* NATUSPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* NATUSPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
