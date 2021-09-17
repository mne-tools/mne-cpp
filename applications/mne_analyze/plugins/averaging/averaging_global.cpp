#include "averaging_global.h"

const char* AVERAGINGPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* AVERAGINGPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* AVERAGINGPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
