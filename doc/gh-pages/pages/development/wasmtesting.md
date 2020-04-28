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

## Trigger the Github Actions workflow

Create a new branch named `wasm`. If you decide to use a different name make sure to change the branch parameter in the `wasmtest.yml` workflow file accordingly. When ready, commit and push your changes to your remote. This will trigger a wasm build being build via Github actions and then be pushed to your fork's `gh-pages` branch. Please note that this will delete everyhting presenet in your `gh-pages` branch. Building Qt and MNE-CPP for Wasm takes quite some time because we only have two CPU cores at our disposal.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch. Once the GitHub action finishes you can take a look at your changes by visiting `https://$username.github.io/mne-cpp/mne_analyze.html`. Please note that it can take some time or multiple refreshes for the changes to show.