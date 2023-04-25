//=============================================================================================================
/**
 * @file     ipfinder.cpp
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the definition of the Fieldline class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include <iostream>
#include <regex>
#include <thread>
#include <cstdlib>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "ipfinder.h"
#include "ipfinder_network_unix.h"

namespace IPFINDER {

const std::regex IP_REGEX("[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}");
const char* default_table_file = "~/.ipfinder_171921";
const int defaultNumRetriesMax = 3;

void printMacIpList(const std::vector<MacIp>& macIpList) {
  for (const MacIp& macip_i : macIpList) {
    std::cout << "mac: " << macip_i.mac << " - ip: "
      << (macip_i.ip.empty() ? "not found" : macip_i.ip)
      << "\n";
  }
}

void sendPingsAroundNetwork() {
  std::string appStr("ping");
  std::string appFlags("-c 1 -w 0.002");
  std::vector<std::thread> threads;
  std::vector<Network> networks = getNetworksClassC();

  for (auto& netw_i : networks) {
    for (auto& hostIp : netw_i.hostIPs) {
      std::string commandStr = appStr + " " + appFlags + " " + hostIp + " &>/dev/null";
      threads.emplace_back([=]{std::system(commandStr.c_str());});
    }
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

void systemCalltoFile(const std::string& call, const std::string& filename) { 
  std::string callStr(call);
  callStr.append(" > ").append(filename);
  std::system(callStr.c_str());
}

void delete_file(const std::string& filename) {
  std::string command("rm -f");
  std::system(command.append(" ").append(filename).c_str());
}

IpFinder::IpFinder()
  : numRetriesMax(defaultNumRetriesMax),
  numRetries(0)
{  
  arp_table_filename = generateRandomArpTableFileName();
}

//void IpFinder::addMacAddress(const std::string& mac) {
//  std::smatch macParsed;
//  if (std::regex_search(mac, macParsed, MAC_REGEX)) {
//    macIpList.emplace_back(MacIp(macParsed[0],""));
//  }
//}

std::string IpFinder::generateRandomArpTableFileName() {
  std::string newFileName(".ipfinder_");
  const std::string charset = "abcdefghijklmpqrstuvwxyz0123456789";
  const int strLen(8);

  for(int i =0; i < strLen; i++) {
    int index = rand() % charset.length();
    newFileName += charset[index];
  }
  return newFileName;
}

void IpFinder::addMacAddress(const std::string& mac) {
  std::regex MAC_REGEX("[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}");
  std::smatch macParsed;
  auto bla = std::regex_search(mac, macParsed, MAC_REGEX);
  if (bla) {
    if (macParsed.size() > 0) {
      macIpList.emplace_back(MacIp(macParsed[0],"0.0.0.0"));
    } else {
      std::cerr << "Error: no matches found for MAC address " << mac << std::endl;
    }
  } else {
    std::cerr << "Error: failed to match MAC address " << mac << " with regular expression." << std::endl;
  }
}

void IpFinder::findIps() {
  numRetries += 1;
  findIpsInARPTable();
  if (!allIpsFound() && (numRetries < numRetriesMax)) {
    sendPingsAroundNetwork();
    findIps();
  }
}

void IpFinder::findIpsInARPTable() {
  systemCalltoFile("arp -a", arp_table_filename);
  std::ifstream fp(arp_table_filename);
  if (!fp.is_open()) {
    std::cout << "Error: Unable to open arp file table.\n";
    std::cout.flush();
    return;
  } 
  std::string line;
  for (MacIp& macip_i : macIpList) {
    while (std::getline(fp, line)) {
      std::smatch mac_found;
      std::regex mac_i(macip_i.mac);
      if (std::regex_search(line, mac_found, mac_i)) {
        std::smatch ip_found;
        if (std::regex_search(line, ip_found, IP_REGEX)) {
          macip_i.ip = ip_found[0];
        }
      }
    }
  }
  fp.close();
  delete_file(arp_table_filename);
}

bool IpFinder::allIpsFound() const {
  for (const MacIp& macip_i: macIpList) {
    if (macip_i.ip.empty()) {
      return false;
    }
  }
  return true;
}

}  // namepsace IPFINDER

