#include "rtprocessing_global.h"

const char* RTPROCESSINGLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* RTPROCESSINGLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* RTPROCESSINGLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};

