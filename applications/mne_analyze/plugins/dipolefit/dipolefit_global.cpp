#include "dipolefit_global.h"

const char* DIPOLEFITPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* DIPOLEFITPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* DIPOLEFITPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
