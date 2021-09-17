#include "brainflowboard_global.h"

const char* BRAINFLOWBOARDPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* BRAINFLOWBOARDPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* BRAINFLOWBOARDPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};


