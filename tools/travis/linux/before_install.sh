#!/bin/bash
set -ev

# Setup display
export DISPLAY=:99.0
sh -e /etc/init.d/xvfb start

# Install Ubuntu packages

# Install Qt
sudo add-apt-repository ppa:beineri/opt-qt56-trusty -y
sudo apt-get update -qq
