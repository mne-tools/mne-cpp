#ifndef BUILD_INFO_LIB
#define BUILD_INFO_LIB

#include <cstring>

namespace BUILDINFO{

//=============================================================================================================
/**
 * Returns build time (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto timeNowNow()
{
    return __TIME__;
}

//=============================================================================================================
/**
 * Returns build date (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto dateToday()
{
    return __DATE__;
}

//=============================================================================================================
/**
 * Returns build date and time (time preprocessor was run). Must be called in compiled function to return
 * correctly.
 */
constexpr auto dateTimeNow()
{
    return __DATE__ " " __TIME__;
}
//=============================================================================================================
/**
 * @brief githash
 */
constexpr auto gitHash()
{
#ifdef GIT_HASH
    return GIT_HASH;
#else
    return "Git hash not defined.";
#endif
}

constexpr auto gitHashLong()
{
#ifdef GIT_HASH_LONG
    return GIT_HASH_LONG;
#else
    return "Git hash long not defined.";
#endif
}

}

#endif
