---
title: Improving the Documentation
parent: Contribute
nav_order: 2
---
# Improving the Documentation

This page shows you how to contribute to MNE-CPP's website. The easiest way to make changes to the documentation and check them before you do a pull request is to follow the steps below.

## Set up gh-pages

If not present already, create a new branch called `gh-pages`. Please note that the `git rm -rf .` command will delete all files in your repository folder, which are not tracked by git:

    git checkout --orphan gh-pages
    git rm -rf .
    touch README.md
    git add README.md
    git commit -m 'initial gh-pages commit'
    git push origin gh-pages

Make sure that `gh-pages` is activated in your forked MNE-CPP repository by checking: mne-cpp > Settings > Options > GitHub Pages > Source: gh-pages branch.

## Create an access token

Create a new access token and give it repo rights only. A guide on how to create a new token can be found [here](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line).

## Set secrets

Create a secret for your forked MNE-CPP repository. A guide on how to create a new token can be found [here](https://help.github.com/en/actions/automating-your-workflow-with-github-actions/creating-and-using-encrypted-secrets#creating-encrypted-secrets). The secret must be named `GIT_CREDENTIALS_DOCU_TEST` and have the following format `https://$username:$token@github.com/`, where `$token` is the access token created in the step above and `$username` is your GitHub user name.

## Make your changes to the documentation

Create a new branch named `docu`. If you decide to use a different name make sure to change the branch parameter in the workflow file accordingly. The website is maintained via .md files, which can be found in `mne-cpp/doc/gh-pages`. When ready, commit and push your changes to your remote.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch. Once the GitHub action finishes you can take a look at your changes by visiting `http://$username.github.io/mne-cpp/`. Please note that it can take some time or multiple refreshes for the changes to show.