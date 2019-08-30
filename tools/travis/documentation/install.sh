#!/bin/bash

# Install Doxygen
sudo apt-get install -qq qttools5-dev-tools doxygen graphviz

# Move azcopy so we can easily use it
tar -xzvf downloadazcopy-v10-linux
find . -type f -name 'azcopy' -exec mv -t ./ {} +
