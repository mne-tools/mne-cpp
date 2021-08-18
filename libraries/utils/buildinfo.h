#ifndef BUILD_INFO_LIB
#define BUILD_INFO_LIB

#include <cstring>

namespace BUILDINFO{

constexpr auto time()
{
    return __TIME__;
}

constexpr auto date()
{
    return __DATE__;
}

constexpr auto timestamp()
{
    return __TIMESTAMP__;
}

//constexpr auto githash()
//{
//#ifdef GIT_HASH
//    return GIT_HASH;
//#else
//    return "Git hash not defined.";
//#endif
//}

//char* githash_short()
//{

//}

//char* version()
//{

//}

}

#endif
