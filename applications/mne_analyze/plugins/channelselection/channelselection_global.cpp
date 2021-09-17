#include "channelselection_global.h"

const char* CHANNELSELECTIONPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* CHANNELSELECTIONPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* CHANNELSELECTIONPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
