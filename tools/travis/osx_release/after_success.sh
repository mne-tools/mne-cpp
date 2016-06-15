#!/bin/bash

if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then






    #Master
#    if [[ $TRAVIS_BRANCH == 'master' ]]; then
#        # upload artifacts
#        curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/
#    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
#        # upload artifacts
#        curl -u $ONEOO_LOGIN:$ONEOO_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/
#    fi
fi