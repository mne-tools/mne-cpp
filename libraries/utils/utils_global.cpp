#include "utils_global.h"

const char* UTILSLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* UTILSLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* UTILSLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
