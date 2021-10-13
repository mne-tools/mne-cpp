---
title: Contributing to the Website
parent: Contribute
grand_parent: Development
nav_order: 2
---

# Contributing to the Website

This page shows you how to contribute to MNE-CPP's documentation website. As with other code contributions, you will need to fork MNE-CPP's repository in GitHub and then do a pull request.

The content of this website is actually stored inside the code repository itself. The website is maintained via markdown language in `.md` files, which can be found in `mne-cpp/doc/gh-pages`. The easiest way to make changes to the documentation and check them before you do a pull request is to either use [Visual Studio Code (VS Code)](https://code.visualstudio.com){:target="_blank" rel="noopener"} together with a [Docker](https://www.docker.com){:target="_blank" rel="noopener"} container or to use GitHub Actions instead.

## VS Code together with a Docker container (Recommended)

This solution works with VS Code and Docker installed on your system only. Detailed system requirements can be found [here](https://code.visualstudio.com/docs/remote/containers#_system-requirements){:target="_blank" rel="noopener"}.

1. Open VS Code and click on the lower left corner bar with the ```><``` symbol. This is the ```Remote Window Indicator``` which only shows up if you have the ```Remote Development Extensions``` installed. Select `Reopen in Container`. This might take a while since the needed docker images will be downloaded and a new container created. The first build after Docker installation might fail. If that is the case try closing and reopening VS Code and try again from this step.
2. Go to your web browser and open [http://localhost:4000/doc/gh-pages/](http://localhost:4000/doc/gh-pages/){:target="_blank" rel="noopener"}. You should be able to navigate through the documentation website.
3. Inside VS Code navigate to `doc/gh-pages/pages` and make your changes.
4. Everytime you save your changes in VS Code the website will automatically recompile. Refresh the website in your web browser to see the results.

## GitHub Actions

### Set up gh-pages

If not present already, create a new branch called `gh-pages`. Please note that the `git rm -rf .` command will delete all files in your repository folder, which are not tracked by git:

```
git checkout --orphan gh-pages
git rm -rf .
touch README.md
git add README.md
git commit -m 'initial gh-pages commit'
git push origin gh-pages
```

Make sure that `gh-pages` is activated in your forked MNE-CPP repository settings. You can check: mne-cpp > Settings > Options > GitHub Pages > Source: gh-pages branch.

### Make your Changes to the Documentation

Create a new branch named `docu` and do your changes locally in that branch. When ready, commit and push your changes to your remote repository forked from MNE-CPP's.

If everything was setup correctly, the push should trigger a GitHub action to build your changes to the `gh-pages` branch (the one you created in the previous step). Once the GitHub action finishes, you will be able to take a look at your changes by visiting `http://$username.github.io/mne-cpp/`. Please note that it can take some time or multiple refreshes for the changes to show.

If you decide to use a different branch name, instead of `docu`, to make changes to this website,  make sure to change the branch parameter in the `docutest.yml` file accordingly (this file is stored in `.github/workflows/docutest.yml`).