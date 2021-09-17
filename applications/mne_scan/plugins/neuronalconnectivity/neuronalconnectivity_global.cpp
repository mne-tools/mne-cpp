#include "neuronalconnectivity_global.h"

const char* NEURONALCONNECTIVITYPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* NEURONALCONNECTIVITYPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* NEURONALCONNECTIVITYPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
