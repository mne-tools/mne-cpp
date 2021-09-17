#include "controlmanager_global.h"

const char* CONTROLMANAGERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* CONTROLMANAGERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* CONTROLMANAGERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
