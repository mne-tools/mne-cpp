#include "view3d_global.h"

const char* VIEW3DPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* VIEW3DPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* VIEW3DPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};

