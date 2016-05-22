#!/bin/bash
set -ev

## Setup display; Start virtual X server, from https://docs.travis-ci.com/user/gui-and-headless-browsers/
#export DISPLAY=:99.0
#sh -e /etc/init.d/xvfb start
#sleep 3 # give xvfb some time to start

# Update Repositories
brew update
