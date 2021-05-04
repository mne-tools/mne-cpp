---
title: Contribution Guide
parent: Contribute
grand_parent: Development
nav_order: 1
---
# Contribution Guide

MNE-CPP is an open-source project and is made better by contributions from our users. This guide should help you through the process of contributing to the project:

1. Setup MNE-CPP to [build from source](buildguide.md).
2. Familiarize yourself with our [coding conventions](contr_style.md).
3. Create a new feature-related branch. Do not issue a Pull Request from your fork's main branch. Make your changes to that feature-branch commit.
4. Run tests locally via calling the testing scripts in `tools/testing`. Make sure you have the `bin/mne-cpp-test-data` submodule enabled. 
5. Check if your changes pass the cross-platform CI tests by running `git push origin <yourBranchName>:testci -f`. Check the Actions tab on your forked MNE-CPP repository's Github page and fix possible problems.
6. Push your changes to your remote (forked GitHub) repository.
7. Once all CI tests pass, go to your remote (forked GitHub) repository via the GitHub website and create a pull request:
   * Right next to the branch selection tool, look for the New pull request button and click it.
   * The Compare Changes page will appear.
   * Select the appropriate base and compare branch . The base branch is the branch of the repository you want to merge your changes to (by default the base branch is a branch of the repository you forked from). The compare branch is the branch where you implemented the new feature(s), bugfix(es), etc.
   * Name the pull request, describe your changes and hit Create pull request button.
   * For a more detailed overview of how to make a pull request, we recommend checking out the [guide on the official GitHub website](https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project){:target="_blank" rel="noopener"}.
8. After you created the pull request, wait for the [Continuous Integration](ci.md) checks and subsequent review process to finish:
   * MNE-CPP developers will review your code.
   * Please change your code on your local machine based on the comments made by the reviewers. Note: There is no need to create a new pull request for every new change you make to your code. Just commit the changes and push your code to your remote (forked GitHub) repository. The changes will automatically be added to your already created pull request.
   * As soon as your code suits all of the MNE-CPP coding conventions and possible inconsistencies were dealt with, your code will be merged into the main MNE-CPP repository.
9. Congratulations! You are now an official contributor to the MNE-CPP project.
