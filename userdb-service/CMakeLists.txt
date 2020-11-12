set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

pkg_check_modules(Glibmm REQUIRED glibmm-2.4>=2.42.0)
pkg_check_modules(Giomm REQUIRED giomm-2.4>=2.42.0)

include(GenerateStub)

generate_stub("${CMAKE_CURRENT_SOURCE_DIR}/com.example.UserDb.xml" ${GENERATED_DIR} "userdb")

add_executable(userdb-service main.cpp)
target_include_directories(userdb-service PRIVATE ${Glibmm_INCLUDE_DIRS} ${Giomm_INCLUDE_DIRS})
target_compile_options(userdb-service PRIVATE ${Glibmm_CFLAGS_OTHER} ${Giomm_CFLAGS_OTHER})
target_link_libraries(userdb-service PRIVATE ${Glibmm_LIBRARIES} ${Giomm_LIBRARIES} userdb-stub)
