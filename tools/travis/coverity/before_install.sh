#!/bin/bash

# Update Repositories
sudo add-apt-repository ppa:beineri/opt-qt571-trusty -y
sudo apt-get update -qq

#install missing Coverity certificate; ToDo: this will be obsolete soon.
echo -n | openssl s_client -connect scan.coverity.com:443 | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | sudo tee -a /etc/ssl/certs/ca-certificates.crt
