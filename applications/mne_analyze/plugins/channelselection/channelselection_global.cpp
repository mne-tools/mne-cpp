#include "channelselection_global.h"

const char* CHANNELSELECTIONPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* CHANNELSELECTIONPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* CHANNELSELECTIONPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
