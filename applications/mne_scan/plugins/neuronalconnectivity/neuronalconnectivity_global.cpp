#include "neuronalconnectivity_global.h"

const char* NEURONALCONNECTIVITYPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* NEURONALCONNECTIVITYPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* NEURONALCONNECTIVITYPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
