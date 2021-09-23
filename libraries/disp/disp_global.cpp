#include "disp_global.h"

const char* DISPLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* DISPLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* DISPLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
