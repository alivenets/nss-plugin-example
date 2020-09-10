#include <sdbus-c++/sdbus-c++.h>

#include <iostream>
#include <string>

sdbus::IObject *g_echoService{};

std::string echo(std::string in)
{
    std::cout << "Echo: " << in << std::endl;
    return in;
}

int main(void)
{
    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "com.example.EchoService";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    // Create concatenator D-Bus object.
    const char* objectPath = "/com/example/EchoService";
    auto echoService = sdbus::createObject(*connection, objectPath);

    g_echoService = echoService.get();

    // Register D-Bus methods and signals on the concatenator object, and exports the object.
    const char* interfaceName = "com.example.EchoService";
    echoService->registerMethod("Echo").onInterface(interfaceName).implementedAs(&echo);
    echoService->finishRegistration();

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
    return 0;
}
