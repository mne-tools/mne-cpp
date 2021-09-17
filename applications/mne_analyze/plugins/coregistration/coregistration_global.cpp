#include "coregistration_global.h"

const char* COREGISTRATIONPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* COREGISTRATIONPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* COREGISTRATIONPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
