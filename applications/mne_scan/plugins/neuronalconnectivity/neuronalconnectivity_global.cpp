#include "neuronalconnectivity_global.h"

const char* NEURONALCONNECTIVITYPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* NEURONALCONNECTIVITYPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* NEURONALCONNECTIVITYPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
