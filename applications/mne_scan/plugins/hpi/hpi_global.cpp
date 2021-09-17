#include "hpi_global.h"

const char* HPIPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* HPIPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* HPIPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
