---
title: Coding Conventions and Style Guide
parent: Contribute
nav_order: 2
---
# Coding Conventions and Style Guide

### Naming Conventions

Please make use of the following coding conventions when contributing to MNE-CPP.

|Object|Rule|
| --------------- | ------------------- |
|Namespace 	      |MYNAMESPACE          |
|Classes 	        |MyClass              |
|Member functions |myFunction           |
|Member variables |m_typeMeaningfulName |
|Local variables 	|typeMeaningfulName   |

### Command Line Outputs

Please make use of qDebug() for debugging, qInfo() for user information and qWarning for user warning purposes. qFatal() should only be used to initiate a total application stop. Please note that Eigen structures can only be plotted via std::cout (you therefore need to add #include<iostream.h>).

### Readability and Understandability

 * Avoid complex and condensed expressions
 * Use meaningful variable names

### Documentations
 * Please use the DoxyGen style to document your code.

## Commit Policy

 1. If you add new functions/classes, ensure everything is documented properly.
 2. All code should follow the Coding Conventions & Style.
 3. Write new unit tests for the bugs you fixed or functionality you added.
 4. Commit often! In particular: Make atomic commits. This means that each commit should contain exactly one self-contained change - do not mix unrelated changes, and do not create inconsistent states. Never "hide" unrelated fixes in bigger commits.
 5. Use task related flags in front of each commit. I.e. if your task has the task tag SC-1, put [SC-1] in front of the commit message.
 6. Write descriptive commit messages. Make them self-contained, so people do not have to research the historical context to make sense of them.
 7. And most importantly: use your brain :)

## Setup the MNE-CPP QtCreator wizard

The following steps will show you how to setup the MNE-CPP QtCreator wizard. This wizard is of great help to conveniently create new C++ classes and sub-projects which are conform with all the MNE-CPP coding conventions. This helps a great deal when contributing your code to the MNE-CPP main repository at a later point in time.

 1. Navigate to your MNE-CPP repository `\tools\coding_conventions\qtCreator_wizard`
 2. Copy the mnecpp folder to your QtCreator installation folder `\share\qtcreator\templates\wizards\`. Usually QtCreator is installed in your Qt distribution's `\Tools` folder, e.g., `C:\Qt\Tools\QtCreator\share\qtcreator\templates\wizards`.
 3. Restart QtCreator.
 4. Now you should be able to right click on the project where you want to add a new class and see the MNE-CPP category appear in the wizard.
