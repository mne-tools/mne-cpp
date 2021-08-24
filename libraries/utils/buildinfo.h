#ifndef BUILD_INFO_LIB
#define BUILD_INFO_LIB

#include <cstring>

namespace BUILDINFO{

//=============================================================================================================
/**
 * Returns build time (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto time()
{
    return __TIME__;
}

//=============================================================================================================
/**
 * Returns build date (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto date()
{
    return __DATE__;
}

//=============================================================================================================
/**
 * Returns date and time of modification of source file. Must be called in compiled function to return correctly.
 */
constexpr auto timestamp()
{
    return __TIMESTAMP__;
}

//=============================================================================================================
/**
 * @brief githash
 */
constexpr auto githash()
{
#ifdef GIT_HASH
    return GIT_HASH;
#else
    return "Git hash not defined.";
#endif
}

//char* githash_short()
//{

//}

//char* version()
//{

//}

}

#endif
