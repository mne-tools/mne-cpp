#include <string>

namespace BUILDTIME{

struct BuildTimestamp{
    constexpr auto time(){return __TIME__;};
    constexpr auto date(){return __DATE__;};
};

constexpr BuildTimestamp get(){return BuildTimestamp();};

}
