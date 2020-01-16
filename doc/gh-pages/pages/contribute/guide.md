---
title: Contribution Guide
parent: Contribute
nav_order: 1
---
# Contribution Guide

MNE-CPP is an open-source project and is made better by contributions from our users. To start contributing, make sure to first follow our [guide](../install/buildguide.md) to setup MNE-CPP for development. You can also view our overview on [deploying MNE-CPP](build_run_depl_pack.md), and our [convention and style guide](conv_style.md).

1. Read the [coding conventions and style guide](conv_style.md).
2. Create a new branch and make your changes to the code (please follow the coding conventions) and do a commit (please follow the commit policy).
3. Push your changes to your remote (forked Git Hub) repository.
4. Go to your remote (forked Git Hub) repository via the GitHub website and create a pull request:
   * Right next to the branch selection tool, look for the New pull request button and click it.
   * The Compare Changes page will appear.
   * Select the appropriate base and compare branch . The base branch is the branch of the repository you want to merge your changes to (by default the base branch is a branch of the repository you forked from). The compare branch is the branch where you implemented the new feature(s), bugfix(es), etc.
   * Name the pull reqest, describe your changes and hit Create pull request button.
   * For a more detailed overview of how to make a pull request, we recommend checking out the [guide on the official GitHub website](https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project).
5. After you created the pull request, wait for the peer review process to be finished:
   * Two MNE-CPP developers will review your code.
   * Please change your code on your local machine based on the comments made by the reviewers. Note: There is no need to create a new pull request for every new change you make to your code. Just commit the changes and push your code to your remote (forked Git Hub) repository. The changes will automatically be added to your already created pull request.
   * As soon as your code suits all of the MNE-CPP coding conventions and possible inconsistencies were dealt with, your code will be merged into the main MNE-CPP repository.
6. Congratulations! You are now an official contributor to the MNE-CPP project.