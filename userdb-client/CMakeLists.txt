find_package(PkgConfig REQUIRED)

pkg_check_modules(Glib REQUIRED glib-2.0)
pkg_check_modules(Gio REQUIRED gio-2.0)

add_library(userdb-client-common OBJECT client.c)
target_include_directories(userdb-client-common PRIVATE ${Glib_INCLUDE_DIRS} ${Gio_INCLUDE_DIRS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(userdb-client-common PRIVATE ${Glib_CFLAGS_OTHER} ${Gio_CFLAGS_OTHER} -fPIC)
target_link_libraries(userdb-client-common PRIVATE ${Glib_LIBRARIES} ${Gio_LIBRARIES})

add_executable(userdb-client-test main.c)
target_link_libraries(userdb-client-test PRIVATE userdb-client-common)
