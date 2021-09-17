#include "connectivity_global.h"

const char* CONNECTIVITYLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* CONNECTIVITYLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* CONNECTIVITYLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
