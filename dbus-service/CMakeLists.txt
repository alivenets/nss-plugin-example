find_package(sdbus-c++ REQUIRED)

add_executable(dbus-service main.cpp)
target_link_libraries(dbus-service PRIVATE SDBusCpp::sdbus-c++)
install(FILES com.example.EchoService.conf DESTINATION /etc/dbus-1/system.d/)
