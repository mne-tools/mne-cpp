#include "inverse_global.h"

const char* INVERSELIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* INVERSELIB::buildHash(){ return BUILDINFO::gitHash();};

const char* INVERSELIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
