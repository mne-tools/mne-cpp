#include <string>
#include <vector>

#ifndef IPFINDER_GETNETWORKS_H
#define IPFINDER_GETNETWORKS_H

namespace IPFINDER {

struct Network {
    std::string baseAddress;
    int numOfHosts;
    std::vector<std::string> hostIPs;
};

void printNetworks(const std::vector<Network>& networks);

std::vector<Network> getNetworksClassC(); 

}  // namespace IPFINDER

#endif  // IPFINDER_GETNETWORKS_H
