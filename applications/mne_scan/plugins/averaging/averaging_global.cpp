#include "averaging_global.h"

const char* AVERAGINGPLUGIN::BUILD_TIME(){ return BUILDINFO::time();};

const char* AVERAGINGPLUGIN::BUILD_DATE(){ return BUILDINFO::date();};

const char* AVERAGINGPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* AVERAGINGPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};