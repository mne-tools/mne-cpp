#include "rawdataviewer_global.h"

const char* RAWDATAVIEWERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* RAWDATAVIEWERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* RAWDATAVIEWERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
