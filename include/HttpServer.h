#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <memory>
#include "AirlineManager.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int socket_t;
#endif

class HttpServer {
private:
    int port;
    std::shared_ptr<AirlineManager> airlineManager;
    bool isRunning;
    socket_t serverSocket;

    void handleClient(socket_t clientSocket);
    std::string getMimeType(const std::string& path);
    std::string readFileContent(const std::string& filePath);
    void sendResponse(socket_t clientSocket, int statusCode, const std::string& statusText,
                      const std::string& contentType, const std::string& body);

public:
    HttpServer(int port, std::shared_ptr<AirlineManager> manager);
    ~HttpServer();

    void start();
    void stop();
};

#endif // HTTP_SERVER_H
