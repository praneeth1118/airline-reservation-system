#include <iostream>
#include <memory>
#include "../include/AirlineManager.h"
#include "../include/HttpServer.h"

int main() {
    std::cout << "Initializing Airline Reservation System..." << std::endl;

    // Instantiate System Manager
    auto airlineManager = std::make_shared<AirlineManager>();

    // Start Embedded C++ Web Server on port 8080
    int port = 8080;
    HttpServer server(port, airlineManager);
    server.start();

    return 0;
}

