---
title: Git Workflow
parent: Contribute
nav_order: 4
---
# Git Workflow

This part is focused on giving a short overview of git commands that are most likely to cover the general workflow. For further information and more advanced use, please check google.

First steps as desribed in [Build from Source](https://mne-cpp.github.io/pages/install/buildguide.html){:target="_blank" rel="noopener"}:
```
   $ git clone https://github.com/<YourGitUserName>/mne-cpp.git
   $ git remote add upstream https://github.com/mne-tools/mne-cpp.git
   $ git fetch --all 
   $ git rebase upstream/master
```
After that, the general workflow is covered by following steps. For more advanced problems, please consult google. 

- Create new branch from `master`: 
```
   $ git checkout -b <branch_name> master
```
- After you applied changes, check the status to get an overview on changes: 
```
   $ git status
```
- Add unstaged changes (colored in red) to prepare next commit: 
```
   $ git add <changed_file_name>
   or
   $ git add --all
```
- Commit the added files (colored in green in status report) and add a meaningfull message about what changed and why (have a look at our [commit policy](https://rdoerfel.github.io/mne-cpp/pages/contribute/conv_style.html){:target="_blank" rel="noopener"} for that: 
```
   $ git commit -m "meaningful commit message" 
   or 
   $ git commit --all
```
- If you make small changes that are related to the previous commit, add your changes to the previous commit with: 
```
   $ git commit --amend
```
- Push your changes to origin (your fork in your GitHub page): 
```
   $ git push origin <branch_name>
```
Please check if there are changes in upstream/master (the mne-cpp Github repository) before you open a new Pull Request. If there are changes apply following steps:

- Get latest changes: 
```
   $ git fetch --all 
   $ git fetch upstream/master` 
```
- Solve [merge conflicts](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/resolving-a-merge-conflict-using-the-command-line){:target="_blank" rel="noopener"}, if they occure. 
- [Rebase](https://www.atlassian.com/git/tutorials/rewriting-history/git-rebase){:target="_blank" rel="noopener"} your branch to set all your commits on top of the masters history: 
```
   $ git checkout <branch_to_rebase>
   $ git rebase upstream/master
```