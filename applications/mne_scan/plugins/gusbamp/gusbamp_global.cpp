#include "gusbamp_global.h"

const char* GUSBAMPPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* GUSBAMPPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* GUSBAMPPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
