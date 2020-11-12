cmake_minimum_required(VERSION 3.10)

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_CXX_STANDARD 17)

project(nss-plugin-example)

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})
include_directories(${GENERATED_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

add_subdirectory(dbus-service)
add_subdirectory(userdb-client)
add_subdirectory(nss-plugin)
add_subdirectory(userdb-service)
