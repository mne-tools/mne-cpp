#include "rawdataviewer_global.h"

const char* RAWDATAVIEWERPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* RAWDATAVIEWERPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* RAWDATAVIEWERPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
