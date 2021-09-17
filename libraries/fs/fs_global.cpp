#include "fs_global.h"

const char* FSLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FSLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* FSLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
