#include "fiff_global.h"

const char* FIFFLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FIFFLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* FIFFLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
