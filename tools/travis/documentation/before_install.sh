#!/bin/bash

# Update Repositories
sudo add-apt-repository ppa:libreoffice/ppa -y
sudo apt-get update -qq

# Get azcopy
wget https://aka.ms/downloadazcopy-v10-linux
