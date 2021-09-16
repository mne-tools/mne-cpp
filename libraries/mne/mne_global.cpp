#include "mne_global.h"

const char* MNELIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* MNELIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* MNELIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};

