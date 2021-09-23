#include "rawdataviewer_global.h"

const char* RAWDATAVIEWERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* RAWDATAVIEWERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* RAWDATAVIEWERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
