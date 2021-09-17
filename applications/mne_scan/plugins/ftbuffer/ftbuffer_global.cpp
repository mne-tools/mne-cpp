#include "ftbuffer_global.h"

const char* FTBUFFERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FTBUFFERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* FTBUFFERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};


