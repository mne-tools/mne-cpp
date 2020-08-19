---
title: Contributing to the Website
parent: Contribute
nav_order: 2
---
# Contributing to the Website

This page shows you how to contribute to MNE-CPP's documentation website. As with other code contributions, you will need to fork MNE-CPP's repository in github and then do a pull request.

The content in this website is actually stored inside the code repository itself, and the easiest way to make changes to the documentation and check them before you do a pull request is to follow the steps below.

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

Make sure that `gh-pages` is activated in your forked MNE-CPP repository settigns. You can check: mne-cpp > Settings > Options > GitHub Pages > Source: gh-pages branch.

## Make your Changes to the Documentation

Create a new branch named `docu` and do your changes locally in that branch. When ready, commit and push your changes to your remote repository forked from MNE-CPP's.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch (the one you created in the previous step). Once the GitHub action finishes, you will be able to take a look at your changes by visiting `http://$username.github.io/mne-cpp/`. Please note that it can take some time or multiple refreshes for the changes to show.

If you decide to use a different branch name, instead of `docu`, to make changes to this website,  make sure to change the branch parameter in the `docutest.yml` file accordingly (this file is stored in `.github/workflows/docutest.yml`). 

The website is maintained via markdown language in `.md` files, which can be found in `mne-cpp/doc/gh-pages`.



