---
title: Contributing to the Website
parent: Contribute
nav_order: 2
---
# Contributing to the Website

This page shows you how to contribute to MNE-CPP's website. The easiest way to make changes to the documentation and check them before you do a pull request is to follow the steps below.

## Set up gh-pages

If not present already, create a new branch called `gh-pages`. Please note that the `git rm -rf .` command will delete all files in your repository folder, which are not tracked by git:
```console
git checkout --orphan gh-pages
git rm -rf .
touch README.md
git add README.md
git commit -m 'initial gh-pages commit'
git push origin gh-pages
```

Make sure that `gh-pages` is activated in your forked MNE-CPP repository by checking: mne-cpp > Settings > Options > GitHub Pages > Source: gh-pages branch.

## Make your changes to the documentation

Create a new branch named `docu`. If you decide to use a different name make sure to change the branch parameter in the `docutest.yml` workflow file accordingly. The website is maintained via .md files, which can be found in `mne-cpp/doc/gh-pages`. When ready, commit and push your changes to your remote.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch. Once the GitHub action finishes you can take a look at your changes by visiting `http://$username.github.io/mne-cpp/`. Please note that it can take some time or multiple refreshes for the changes to show.