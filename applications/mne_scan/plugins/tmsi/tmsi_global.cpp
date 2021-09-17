#include "tmsi_global.h"

const char* TMSIPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* TMSIPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* TMSIPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
