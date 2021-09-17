#include "eegosports_global.h"

const char* EEGOSPORTSPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* EEGOSPORTSPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* EEGOSPORTSPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
