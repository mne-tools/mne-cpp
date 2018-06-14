#!/bin/bash -xf

if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then
    # === Deployment ===
    TANGIBLES=(mne_scan mne_browse mne_analyze)
    n_elements=${#TANGIBLES[@]}
	
    # === Copy Tangibles ===
    if [[ $TRAVIS_BRANCH == 'master' ]]; then
        for ((i = 0; i < n_elements; i++)); do
            cp "./bin/${TANGIBLES[i]}.dmg" "./"
            tangible_name="${TANGIBLES[i]}-$TRAVIS_BRANCH.dmg"
            mv "./${TANGIBLES[i]}.dmg" $tangible_name
            # upload artifacts
            curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $tangible_name ftp://$REMOTE_SERVER/
        done
    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
        for ((i = 0; i < n_elements; i++)); do
            cp "./bin/${TANGIBLES[i]}.dmg" "./"
            tangible_name="${TANGIBLES[i]}-$TRAVIS_BRANCH.dmg"
            mv "./${TANGIBLES[i]}.dmg" $tangible_name
            # upload artifacts
            curl -u $ONEOO_LOGIN:$ONEOO_PASSWORD -T $tangible_name ftp://$REMOTE_SERVER/
        done
    fi
fi
