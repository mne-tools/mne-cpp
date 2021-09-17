#include "eegosports_global.h"

const char* EEGOSPORTSPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* EEGOSPORTSPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* EEGOSPORTSPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
