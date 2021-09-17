#include "disp_global.h"

const char* DISPLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* DISPLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* DISPLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
