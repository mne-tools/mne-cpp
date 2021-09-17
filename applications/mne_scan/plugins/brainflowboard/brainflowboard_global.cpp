#include "brainflowboard_global.h"

const char* BRAINFLOWBOARDPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* BRAINFLOWBOARDPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* BRAINFLOWBOARDPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};


