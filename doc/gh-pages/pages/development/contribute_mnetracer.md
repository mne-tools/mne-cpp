---
title: MNE Tracer
parent: Contribute
grand_parent: Development
nav_order: 5
---
# MNE Tracer

***
                **_It is very hard to improve what you can not measure._**
***

During development it is common to have to decide between two (or more) different implementations. In general in MNE-CPP, we strive for improving code maintainability and coherence on top of achieving an efficient-as-possible implementation. But still, it is not uncommon to find situations when two solutions seem valid. In this scenario, to be able to understand the performance implications of a particular implementation can help understand better the code and obtaine a more informed opinion, therefore learnign through out the process. For this situations, we have developed a code tracer functionality. 

It is also interesting 

MNE Tracer is a single class (MneTracer) which resides in Utils library.

How to use it

Python helper function


```c++
int main(int argc, char *argv[])
{
    MNE_TRACER_ENABLE(hpi_test1.json)

    //... your application starts here

    int returnValue(app.exec());
    
    MNE_TRACER_DISABLE

    return returnValue;
}
```


This part is focused on giving a short overview of git commands that should cover the general git workflow in MNE-CPP. For further information you can check out this [Git tutorial video](https://www.youtube.com/watch?v=DtLrWCFaG0A&feature=youtu.be){:target="_blank" rel="noopener"}.

The first steps to get started, as described in the [build guide](buildguide.md), are:
```
git clone https://github.com/<YourGitUserName>/mne-cpp.git
git remote add upstream https://github.com/mne-tools/mne-cpp.git
git fetch --all
git rebase upstream/master
```

The general workflow is covered by the following steps:

- Create a new branch from `master`:

```
git checkout -b <branchName> master
```

- Get the latest changes and [rebase](https://www.atlassian.com/git/tutorials/rewriting-history/git-rebase){:target="_blank" rel="noopener"}:

```
git fetch upstream
git rebase upstream/master
```

- Solve [merge conflicts](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/resolving-a-merge-conflict-using-the-command-line){:target="_blank" rel="noopener"}, if they occure.

- Make your changes and check the status:

```
git status
```

- Add unstaged changes (colored in red) to prepare next commit:

```
git add <changedFileName>
   or
git add --all
```

- Commit the added files (colored in green in status report) and add a meaningful message about what changed and why (have a look at our [commit policy](contr_style.md)):

```
git commit -m "Fix: meaningful commit message"
   or
git commit --all
```

- If you make small changes that are related to the previous commit, you can amend your changes to the previous commit with:

```
git commit -m "Fix: meaningful commit message" --amend
```

- Push your changes to origin (your MNE-CPP fork on GitHub). Pleaes note that a force push via `-f` might be necessary if you rebased:

```
git push origin <branchName>
   or
git push origin <branchName> -f
```
