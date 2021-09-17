#include "fwd_global.h"

const char* FWDLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FWDLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FWDLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
