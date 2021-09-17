#include "filtering_global.h"

const char* FILTERINGPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FILTERINGPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* FILTERINGPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
