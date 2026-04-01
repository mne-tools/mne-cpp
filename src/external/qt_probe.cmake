# Minimal CMake probe for Qt compatibility
cmake_minimum_required(VERSION 3.15)
project(QtProbe LANGUAGES CXX)

set(REQUIRED_QT_COMPONENTS Core Concurrent Gui Network Widgets Svg Xml PrintSupport ShaderTools GuiPrivate)
find_package(Qt6 REQUIRED COMPONENTS ${REQUIRED_QT_COMPONENTS})
