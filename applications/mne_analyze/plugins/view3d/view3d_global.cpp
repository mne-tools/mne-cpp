#include "view3d_global.h"

const char* VIEW3DPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* VIEW3DPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* VIEW3DPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};

