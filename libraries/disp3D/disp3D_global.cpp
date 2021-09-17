#include "disp3D_global.h"

const char* DISP3DLIB::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* DISP3DLIB::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* DISP3DLIB::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
