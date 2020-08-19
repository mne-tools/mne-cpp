---
title: Testing via CI
parent: WebAssembly
grand_parent: Develop
nav_order: 2
---
# Testing via CI

This page shows you how to check your wasm build and deploy it to your `gh-pages` branch. 

## Set up gh-pages

If not present already, create a new branch called `gh-pages`. Please note that the `git rm -rf .` command will delete all files in your repository folder, which are not tracked by git:
```
git checkout --orphan gh-pages
git rm -rf .
touch README.md
git add README.md
git commit -m 'initial gh-pages commit'
git push origin gh-pages
```

Make sure that `gh-pages` is activated in your forked MNE-CPP repository by checking: mne-cpp > Settings > Options > GitHub Pages > Source: gh-pages branch.

## Create an Access Token

Create a new access token and give it repo rights only. A guide on how to create a new token can be found [here](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line){:target="_blank" rel="noopener"}.

## Set secrets
A guide on how to create a new token can be found [here](https://help.github.com/en/actions/automating-your-workflow-with-github-actions/creating-and-using-encrypted-secrets#creating-encrypted-secrets){:target="_blank" rel="noopener"}. The secret must be named `GIT_CREDENTIALS_WASM_TEST` and have the following format `https://$username:$token@github.com/`, where `$token` is the access token created in the step above and `$username` is your GitHub user name.

## Trigger the Github Actions Workflow

Create a new branch named `wasm`. If you decide to use a different name make sure to change the branch parameter in the `wasmtest.yml` workflow file accordingly. When ready, commit and push your changes to your remote. This will trigger a wasm build being build via Github actions and then be pushed to your fork's `gh-pages` branch. Please note that this will delete everyhting presenet in your `gh-pages` branch. Building Qt and MNE-CPP for Wasm takes quite some time because we only have two CPU cores at our disposal.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch. Once the GitHub action finishes you can take a look at your changes by visiting `https://$username.github.io/mne-cpp/mne_analyze.html`. Please note that it can take some time or multiple refreshes for the changes to show.