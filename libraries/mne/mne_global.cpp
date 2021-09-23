#include "mne_global.h"

const char* MNELIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* MNELIB::buildHash(){ return UTILSLIB::gitHash();};

const char* MNELIB::buildHashLong(){ return UTILSLIB::gitHashLong();};

