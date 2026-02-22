---
title: Coding Conventions
sidebar_label: Coding Conventions
---

# Coding Conventions

## Naming

Please make use of the following coding conventions when contributing to MNE-CPP:

|Object|Rule|
| --------------- | ------------------- |
|Namespace 	      |`MYNAMESPACE`          |
|Classes 	        |`MyClass`              |
|Member functions |`myFunction`           |
|Member variables |`m_typeMeaningfulName` |
|Local variables 	|`typeMeaningfulName`   |

## Readability and Documentation

Use meaningful variable names and type indicators. Avoid complex as well as condensed expressions. For example:

```cpp
int iNumChs = 306;
QString sChName = "MEG0000";
void loadTxtFile(const QString& sPath);
```

In general, try to make your code match the one around it, e.g., indentations, use of linebreaks, etc. Please use the DoxyGen style to document your code, as seen [here](https://github.com/mne-tools/mne-cpp/blob/main/libraries/connectivity/network/network.h). If you introduce a new function, please make sure to use the `@since` tag to specify the current version you are developing for. Always align your function parameters in .h and .cpp files, for example:

```cpp
//=========================================================================================================
/**
 * Constructs a FileLoader object.
 *
 * @param[in]  iOpenMode            The open mode. 0=ReadWrite, 1=WriteOnly, 1=ReadOnly
 * @param[in]  bSkipEmptyLines      Whether to skip empty lines.
 * @param[in]  sCommentIdentifier   String to identify comments. Default is empty which
 *                                  results in comments being read as normal lines.
 * @since 0.1.1
 */
FileLoader(int iOpenMode,
           bool bSkipEmptyLines,
           const QString& sCommentIdentifier = "");
```

## MNE-CPP Class Templates

MNE-CPP provides class templates under `resources/wizards/mnecpp/` that follow the project's coding conventions. These templates can help you quickly scaffold new classes that are consistent with the existing codebase.

### Using with QtCreator

If you use QtCreator as your IDE, you can install these templates as wizards:

1. Copy the `resources/wizards/mnecpp` folder to your QtCreator wizard directory. The location varies by OS — see the [Qt documentation](https://doc.qt.io/qtcreator/creator-project-wizards.html) for details.
2. Restart QtCreator.
3. The MNE-CPP category will appear in the new file/class wizard.

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

The following examples show how such commit messages could look like:
```
FIX: fix namespace error
ENH: add cHPI in Neuromag Plugin
MAINT: improved GitHubAction workflow for Linux deployment
DOC: add documentation for new amplifier in MNE Scan
```

## MNE-CPP Documentation in QtCreator

You can integrate MNE-CPP's API documentation into QtCreator's Help mode (F1):

 * Download the `mne-cpp-doc-qtcreator.qch` file from the [MNE-CPP releases page](https://github.com/mne-tools/mne-cpp/releases).
 * In QtCreator, go to **Tools → Options → Help → Documentation → Add** and select the `.qch` file.
