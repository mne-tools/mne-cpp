---
title: Build from Source
parent: Development
has_children: true
nav_order: 6
---
# Build from Source

The following guides show you how to build the dynamically and statically linked version of MNE-CPP from source. After building the project you will be able to run the different applications that are part of the project. These will apear as executable files with different extensions, depepending on your operating system.

You can run the compiled applications within QtCreator application (this is the recommended tool also for development and building processes), by pressing the Run button. However, if you want to run directly from within a different command-line tool or through the operating system itself, you will have to resolve all the Qt-related dependencies by generating Qt libraries. MNE-CPP provides a useful automatic deployment script which is saved in ```tools/deployment```. In that folder the ```deploy.bat``` script will run in all platforms and generate the necesary libraries for the correct execution of any of the project's applications. This script can be run from anywhere in the file system, but do not change the script's location within the project's folder structure, since it depends on it for a correct execution. ```deploy.bat``` takes two input arguments: the first argument specifies the type of linkage to use in the project's applications. It can be either ```dynamic``` or ```static```. If empty, the linkage option will be set to ```dynamic```. The second argument specifies if a compressed folder (zip, tar, ...) should be created with all the necessary files to run the project's application.

|**Please note:** We recommend building the dynamically linked version for developing purposes.|
