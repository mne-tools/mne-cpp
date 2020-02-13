---
title: Contribution Guide
parent: Contribute
nav_order: 1
---
# Contribution Guide

MNE-CPP is an open-source project and is made better by contributions from our users. To start contributing, make sure to first follow our [guide](../install/buildguide.md) to setup MNE-CPP for development. You can also view our overview on [deploying MNE-CPP](../development/ci.md), and our [convention and style guide](conv_style.md).

1. Familiarize yourself with our [coding conventions](conv_style.md).
2. Create a new branch ([do not develop on your forked master branch](https://blog.jasonmeridth.com/posts/do-not-issue-pull-requests-from-your-master-branch/)), make your changes and do a commit.
3. Push your changes to your remote (forked Git Hub) repository.
4. Go to your remote (forked Git Hub) repository via the GitHub website and create a pull request:
   * Right next to the branch selection tool, look for the New pull request button and click it.
   * The Compare Changes page will appear.
   * Select the appropriate base and compare branch . The base branch is the branch of the repository you want to merge your changes to (by default the base branch is a branch of the repository you forked from). The compare branch is the branch where you implemented the new feature(s), bugfix(es), etc.
   * Name the pull reqest, describe your changes and hit Create pull request button.
   * For a more detailed overview of how to make a pull request, we recommend checking out the [guide on the official GitHub website](https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project){:target="_blank" rel="noopener"}.
5. After you created the pull request, wait for the peer review process to be finished:
   * MNE-CPP developers will review your code.
   * Please change your code on your local machine based on the comments made by the reviewers. Note: There is no need to create a new pull request for every new change you make to your code. Just commit the changes and push your code to your remote (forked Git Hub) repository. The changes will automatically be added to your already created pull request.
   * As soon as your code suits all of the MNE-CPP coding conventions and possible inconsistencies were dealt with, your code will be merged into the main MNE-CPP repository.
6. Congratulations! You are now an official contributor to the MNE-CPP project.

## Commit Messages and Pull Requests (PR)

For better readability, we want to introduce some conventions for PR titles and commit messages. Please place those short terms in front of your message. The 'PR only' noted short terms show the admin, if the PR is ready for a final review or if the contributer still plans to make changes. 

| Short | Meaning                                       |
|-------|-----------------------------------------------|
| FIX   | bug fix                                       |
| ENH   | enhancement                                   |
| MAINT | maintenance commit (refactoring, typos, etc.) |
| STY   | style fix                                     |
| DOC   | documentation                                 |
| WIP   | Work in Progress - PR only                    |
| MRG   | Ready to merge - PR only                      |

Some examples for a commit message and a PR title:
```
PR:
[WIP][ENH,MAINT,FIX]: cHPI fitting and real-time head monitoring
```
- [WIP] shows that the contributer is still working on the PR respectivly plans to add some changes
- [ENH,MAINT,FIX] gives a first impression about the content of the PR
```
Commit Message:
ENH: enable cHPI in Neuromag Plugin
or
FIX: fix namespace error 
```
- ENH and FIX both give information about the contend and intend of a commit and make it therfor easier to navigate and read the commit history

## Git Workflow

This part is focused on giving a short overview of git commands, that are most likely to cover the general workflow. For further information and more advanced use, please check google.

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
- Commit the added files (colored in green in status report) and add a meaningfull message about what changed and why (See next section for our conventions): 
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