#include "disp_global.h"

const char* DISPLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* DISPLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* DISPLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
