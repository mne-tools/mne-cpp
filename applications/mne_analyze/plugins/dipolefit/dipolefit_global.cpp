#include "dipolefit_global.h"

const char* DIPOLEFITPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* DIPOLEFITPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* DIPOLEFITPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
