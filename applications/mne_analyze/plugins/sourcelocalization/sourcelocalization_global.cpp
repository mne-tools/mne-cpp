#include "sourcelocalization_global.h"

const char* SOURCELOCALIZATIONPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* SOURCELOCALIZATIONPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* SOURCELOCALIZATIONPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
