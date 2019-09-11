#!/bin/bash

# Update Repositories
sudo add-apt-repository ppa:beineri/opt-qt-5.10.1-trusty -y
sudo apt-get update -qq

# Downloading linuxdeployqt from continious release
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
sudo chmod a+x linuxdeployqt-continuous-x86_64.AppImage

# Get azcopy
wget https://aka.ms/downloadazcopy-v10-linux

#creating a directory for linuxdeployqt to create results 
sudo mkdir -p -m777 mne-cpp
