#include "disp3D_global.h"

const char* DISP3DLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* DISP3DLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* DISP3DLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
