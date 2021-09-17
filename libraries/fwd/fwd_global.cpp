#include "fwd_global.h"

const char* FWDLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FWDLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* FWDLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
