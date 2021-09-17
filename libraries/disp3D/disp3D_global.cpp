#include "disp3D_global.h"

const char* DISP3DLIB::buildDateTime(){ return BUILDINFO::dateTime();};

const char* DISP3DLIB::buildHash(){ return BUILDINFO::gitHash();};

const char* DISP3DLIB::buildHashLong(){ return BUILDINFO::gitHashLong();};
