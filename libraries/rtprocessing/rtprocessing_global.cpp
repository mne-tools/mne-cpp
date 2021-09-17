#include "rtprocessing_global.h"

const char* RTPROCESSINGLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* RTPROCESSINGLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* RTPROCESSINGLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};

