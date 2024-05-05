//=============================================================================================================
/**
 * @file     ipfinder.h
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the Fieldline plugin class.
 *
 */

#ifndef IPFINDER_H
#define IPFINDER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <string>
#include <vector>
#include <regex>

namespace IPFINDER {

struct MacIp
{
  MacIp(const std::string& mac_,
        const std::string& ip_,
        bool value)
  : mac(mac_), ip(ip_), valid(value)
  {};
  MacIp()
  : mac(""), ip(""), valid(false)
  {};
  std::string mac;
  std::string ip;
  bool valid;
};

struct Defaults {
  static const char regexIpStr[];
  static const char regexMacStr[];
  static const char tableFile[];
  static const char pingCommand[];
  static const char pingCommandOpts[];
  static const char deleteFileCommand[];
  static const int numRetriesMax;
  static const char arpTableFilePrefix[];
  static const char randomCharset[];
  static const int randomStringLength;
  static const char defaultIpStr[];
};

void printMacIpList(const std::vector<MacIp>& macIpList);

void sendPingsAroundNetwork();

void systemCalltoFile(const std::string& call,
                      const std::string& filename);

void deleteFile(const std::string& filename);

class IpFinder {
 public:
  IpFinder();
  void findIps();
  void addMacAddress(const std::string& mac);
  bool allIpsFound() const;

  std::vector<MacIp> macIpList;

 private:
  std::string generateRandomArpTableFileName();
  std::string arp_table_filename;
  void findIpsInARPTable();
  int numRetriesMax;
  int numRetries;
};

}  // namespace IPFINDER



#endif  // IPFINDER_H

