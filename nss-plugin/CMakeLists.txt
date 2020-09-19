set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)
include(FindPkgConfig)

add_library(nss_example SHARED lib.c)

target_link_libraries(nss_example PUBLIC -Wl,-soname,libnss_example.so.2)
set_target_properties(nss_example PROPERTIES SOVERSION 2)

install(TARGETS nss_example DESTINATION ${CMAKE_INSTALL_LIBDIR})
