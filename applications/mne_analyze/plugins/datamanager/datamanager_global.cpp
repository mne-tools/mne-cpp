#include "datamanager_global.h"

const char* DATAMANAGERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* DATAMANAGERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* DATAMANAGERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
