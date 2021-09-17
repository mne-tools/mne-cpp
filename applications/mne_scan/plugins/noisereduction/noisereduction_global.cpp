#include "noisereduction_global.h"

const char* NOISEREDUCTIONPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* NOISEREDUCTIONPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* NOISEREDUCTIONPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
