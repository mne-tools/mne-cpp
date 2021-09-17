#include "writetofile_global.h"

const char* WRITETOFILEPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* WRITETOFILEPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* WRITETOFILEPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
