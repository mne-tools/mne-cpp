---
title: Contribution Guide
parent: Contribute
nav_order: 1
---
# Contribution Guide

MNE-CPP is an open-source project and is made better by contributions from our users. This guide should help you through the process of contributing to the project:

1. Setup MNE-CPP to [build from source](../install/buildguide.md).
2. Familiarize yourself with our [coding conventions](conv_style.md).
3. Create a new branch ([do not develop on your forked master branch](https://blog.jasonmeridth.com/posts/do-not-issue-pull-requests-from-your-master-branch/{:target="_blank" rel="noopener"})), make your changes and do a commit.
4. Push your changes to your remote (forked Git Hub) repository.
5. Go to your remote (forked Git Hub) repository via the GitHub website and create a pull request:
   * Right next to the branch selection tool, look for the New pull request button and click it.
   * The Compare Changes page will appear.
   * Select the appropriate base and compare branch . The base branch is the branch of the repository you want to merge your changes to (by default the base branch is a branch of the repository you forked from). The compare branch is the branch where you implemented the new feature(s), bugfix(es), etc.
   * Name the pull reqest, describe your changes and hit Create pull request button.
   * For a more detailed overview of how to make a pull request, we recommend checking out the [guide on the official GitHub website](https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project){:target="_blank" rel="noopener"}.
6. After you created the pull request, wait for the [CI pipeline](../development/ci.md) runs and subsequent review process to be finished:
   * MNE-CPP developers will review your code.
   * Please change your code on your local machine based on the comments made by the reviewers. Note: There is no need to create a new pull request for every new change you make to your code. Just commit the changes and push your code to your remote (forked Git Hub) repository. The changes will automatically be added to your already created pull request.
   * As soon as your code suits all of the MNE-CPP coding conventions and possible inconsistencies were dealt with, your code will be merged into the main MNE-CPP repository.
7. Congratulations! You are now an official contributor to the MNE-CPP project.
