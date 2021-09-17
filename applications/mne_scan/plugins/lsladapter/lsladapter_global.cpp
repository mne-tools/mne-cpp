#include "lsladapter_global.h"

const char* LSLADAPTERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* LSLADAPTERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* LSLADAPTERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
