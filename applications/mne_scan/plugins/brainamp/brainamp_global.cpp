#include "brainamp_global.h"

const char* BRAINAMPPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* BRAINAMPPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* BRAINAMPPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
