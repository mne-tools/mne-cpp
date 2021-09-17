#include "mne_global.h"

const char* MNELIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* MNELIB::buildHash(){ return BUILDINFO::gitHash();};

const char* MNELIB::buildHashLong(){ return BUILDINFO::gitHashLong();};

