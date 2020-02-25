---
title: Coding Conventions
parent: Contribute
nav_order: 3
---
# Coding Conventions

## Naming Conventions

Please make use of the following coding conventions when contributing to MNE-CPP:

|Object|Rule|
| --------------- | ------------------- |
|Namespace 	      |`MYNAMESPACE`          |
|Classes 	        |`MyClass`              |
|Member functions |`myFunction`           |
|Member variables |`m_typeMeaningfulName` |
|Local variables 	|`typeMeaningfulName`   |

## Readability

Use meaningful variable names and type indicators. Avoid complex as well as condensed expressions. For example:

```cpp
int iNumChs = 306;
QString sChName = "MEG0000";
void loadTxtFile(const QString& sPath);
```

In general, try to make your code match the one around it, e.g., indentations, use of linebreaks, etc. Always align your function parameters in .h and .cpp files, for example:

```cpp
//=========================================================================================================
/**
 * Constructs a NetworkEdge object.
 *
 * @param[in]  iValueA          Describe value iValueA.
 * @param[in]  fValueB          Describe value fValueB.
 * @param[in]  matValueC        Describe value matValueC.
 * 
 * @return Describe return value.
 */
int NetworkEdge(int iValueA,
                float fValueB,
                const Eigen::MatrixXd& matValueC);
```

## Documentation

Please use the DoxyGen style to document your code. For an example see [here](https://github.com/mne-tools/mne-cpp/blob/master/libraries/connectivity/network/network.h). 

## Command Line Outputs

Every output should start with `[<ClassName::FunctionName>]`. Please make use of `qDebug()` during development and `qInfo()` for general user information. `qWarning()` should be used to alert about unusual situations which do not lead to a termination of the application. `qCritical()` should only be used if an error was catched which will lead to the application being terminated. For example:

```cpp
void FileLoader::loadTxtFile(const QString& sPath)
{
    qInfo() << "[FileLoader::loadTxtFile] Working on file" << sPath;

    if(!sPath.contains(".txt")) {
        qWarning() << "[FileLoader::loadTxtFile] The file does not end with .txt."
    }

    QFile file(sPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[FileLoader::loadTxtFile] Unable to open file."
        return;
    }
    ...
}
```

| **Please note:** Eigen objects (`MatrixXd`, `VectorXd`, etc.) can only be plotted via `std::cout`.|

## Commit Policy

A good commit should follow: 

 * If you add new functions/classes, ensure everything is documented properly.
 * All code should follow the Coding Conventions & Style.
 * Write new unit tests for the bugs you fixed or functionality you added.
 * Commit often! In particular: Make atomic commits. This means that each commit should contain exactly one self-contained change - do not mix unrelated changes, and do not create inconsistent states. Never "hide" unrelated fixes in bigger commits.
 * Write descriptive commit messages. Make them self-contained, so people do not have to research the historical context to make sense of them.
 * And most importantly: use your brain :)

For better readability, we introduced conventions for PR naming and commit messages. This gives a first impression about the content of a commit and improves the commit history's readability. Please use the following identifiers:

| Short | Meaning                                       |
|-------|-----------------------------------------------|
| FIX   | bug fix                                       |
| ENH   | enhancement (new features, etc.)              |
| MAINT | maintenance commit (refactoring, typos, style fixes, etc.) |
| DOC   | documentation                                 |

The following examples show how such a commit message could look like.
```
FIX: fix namespace error 
ENH: add cHPI in Neuromag Plugin
MAINT: improved GithubAction workflow for Linux deployment
DOC: add documentation for new amplifier in MNE Scan
```

## Setup the MNE-CPP QtCreator wizard

The following steps will show you how to setup the MNE-CPP QtCreator wizard. This wizard is of great help to create new C++ classes which are conform with the MNE-CPP coding conventions. This helps streamlining the contribution process.

 1. Navigate to your MNE-CPP repository `\tools\wizards`
 2. Copy the `mnecpp` folder to your QtCreator installation folder `\share\qtcreator\templates\wizards\`. Usually QtCreator is installed in your Qt distribution's `\Tools` folder, e.g., `<QtFolder>\Tools\QtCreator\share\qtcreator\templates\wizards`.
 3. Restart QtCreator.
 4. Now you should be able to right click on the project where you want to add a new class and see the MNE-CPP category appear in the wizard.

## Add MNE-CPP documentation to QtCreator

You can display external documentation in the Help mode, which you can open via pressing the F1 key. To augment or replace the documentation that ships with Qt Creator and Qt:

 * Download the .qch file `mne-cpp-doc-qtcreator.qch` file for the MNE-CPP version you use [here](https://github.com/mne-tools/mne-cpp/releases){:target="_blank" rel="noopener"}.
 * To add the .qch file to Qt Creator, select Tools > Options > Help > Documentation > Add.
