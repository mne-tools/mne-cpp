#!/bin/bash
set -ev

# Update Repositories
sudo add-apt-repository ppa:beineri/opt-qt56-trusty -y
sudo apt-get update -qq
