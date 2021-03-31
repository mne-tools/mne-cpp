---
title: Build from Source
parent: Development
has_children: true
nav_order: 6
---
# Build from Source

The following guides show you how to build the dynamically and statically linked version of MNE-CPP from source. After building the project you will be able to run the different applications that are part of the project. These will apear as executable files with different extensions, depepending on your operating system.

You can run the compiled applications within QtCreator application (this is the recommended tool also for development and building processes), by pressing the Run button. However, if you want to run directly from within a different command-line tool or through the operating system itself, you will have to resolve all the Qt-related dependencies by generating Qt libraries. MNE-CPP provides a useful automatic deployment script which is saved in ```tools/deployment```. For more information about this script see the [Continous Integration](ci_deployment.md) section. Briefly, run the script ```deploy.bat```  with either ```dynamic``` or ```static``` as input argument to specify the linkage to be used. This way all the necessary Qt-related dependencies will be solved and the applications readily available for execution in all contexts.

|**Please note:** We recommend building the dynamically linked version for developing purposes.|
