#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace IPFINDER {

struct Network {
    std::string baseAddress;
    int numOfHosts;
    std::vector<std::string> hostIPs;
};

void printNetworks(const std::vector<Network>& networks) {
    std::cout << "Networks:" << std::endl;
    for (const auto& network : networks) {
        std::cout << "Base Address: " << network.baseAddress << "\n";
        std::cout << "Number of Hosts: " << network.numOfHosts << "\n";
        std::cout << "Host IPs: ";
        for (int i = 0; i < 10; i++) {
            std::cout << " " << i << " " << network.hostIPs[i] << "\n";
            if (i == 10) break;
        }
    }
}

std::vector<Network> getNetworksClassC() {
    std::vector<Network> networks;
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    void* tmpAddrPtr = nullptr;

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::string ip_address(addressBuffer);

            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::string subnet_mask(addressBuffer);

            // Calculate the network base address
            struct in_addr network_addr;
            inet_aton(ip_address.c_str(), &network_addr);
            struct in_addr mask_addr;
            inet_aton(subnet_mask.c_str(), &mask_addr);
            network_addr.s_addr &= mask_addr.s_addr;
            std::string network_address(inet_ntoa(network_addr));

            // Calculate the number of possible hosts in the network
            // int num_of_bits = 32 - __builtin_popcount(mask_addr.s_addr);
            std::bitset<32> mask(mask_addr.s_addr);
            int num_of_bits = 32 - mask.count();
            int num_of_hosts = (1 << num_of_bits) - 2;            // Calculate the IP addresses of all possible hosts in the network

            if (num_of_hosts > 255) 
              continue;

            std::vector<std::string> host_ips;
            for (int i = 1; i < num_of_hosts; ++i) {
                struct in_addr host_addr;
                host_addr.s_addr = network_addr.s_addr + htonl(i);
                std::string host_ip(inet_ntoa(host_addr));
                host_ips.push_back(host_ip);
            }

            Network network = { network_address, num_of_hosts, host_ips };
            networks.push_back(network);
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return networks;
}

}  // namespace IPFINDER
