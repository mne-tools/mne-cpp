#!/bin/bash

# Install Doxygen
sudo apt-get install -qq qttools5-dev-tools doxygen graphviz

# Instal lgit lfs
curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
sudo apt-get install git-lfs
git lfs install
