#include "gusbamp_global.h"

const char* GUSBAMPPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* GUSBAMPPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* GUSBAMPPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
