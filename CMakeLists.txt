cmake_minimum_required(VERSION 3.16)

# ESP-IDF project integration — requires IDF_PATH to be set in the environment.
# Source the IDF environment before building:
#   . $IDF_PATH/export.sh
#
# Build with:
#   idf.py build
#
# For host-native tests see tests/ (CMake + Conan).
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(stembot VERSION 2.0.0)
