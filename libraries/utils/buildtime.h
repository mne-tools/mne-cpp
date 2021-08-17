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

//char* githash()
//{

//}

//char* githash_short()
//{

//}

//char* version()
//{

//}

}

#endif
