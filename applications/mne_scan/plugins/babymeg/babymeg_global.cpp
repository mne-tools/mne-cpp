#include "babymeg_global.h"

const char* BABYMEGPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* BABYMEGPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* BABYMEGPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
