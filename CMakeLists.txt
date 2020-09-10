cmake_minimum_required(VERSION 3.10)

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_CXX_STANDARD 17)

project(nss-plugin-example)

find_package(PkgConfig REQUIRED)

add_subdirectory(dbus-service)
