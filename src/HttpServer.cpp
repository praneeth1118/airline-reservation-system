#include "../include/HttpServer.h"
#include "../include/JsonUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#pragma comment(lib, "ws2_32.lib")
#endif

HttpServer::HttpServer(int port, std::shared_ptr<AirlineManager> manager)
    : port(port), airlineManager(manager), isRunning(false), serverSocket(INVALID_SOCKET) {}

HttpServer::~HttpServer() {
    stop();
}

std::string HttpServer::getMimeType(const std::string& path) {
    if (path.rfind(".html") != std::string::npos || path.rfind(".htm") != std::string::npos) return "text/html; charset=utf-8";
    if (path.rfind(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.rfind(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.rfind(".json") != std::string::npos) return "application/json; charset=utf-8";
    if (path.rfind(".xlsx") != std::string::npos) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (path.rfind(".xml") != std::string::npos) return "application/xml; charset=utf-8";
    if (path.rfind(".svg") != std::string::npos) return "image/svg+xml";
    if (path.rfind(".png") != std::string::npos) return "image/png";
    if (path.rfind(".jpg") != std::string::npos || path.rfind(".jpeg") != std::string::npos) return "image/jpeg";
    if (path.rfind(".ico") != std::string::npos) return "image/x-icon";
    return "text/plain; charset=utf-8";
}

std::string HttpServer::readFileContent(const std::string& filePath) {
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void HttpServer::sendResponse(socket_t clientSocket, int statusCode, const std::string& statusText,
                              const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    std::string respStr = response.str();
    send(clientSocket, respStr.data(), static_cast<int>(respStr.size()), 0);
}

void HttpServer::handleClient(socket_t clientSocket) {
    char buffer[8192];
    std::string requestStr;
    size_t headerEnd = std::string::npos;

    // Read until headers end
    while ((headerEnd = requestStr.find("\r\n\r\n")) == std::string::npos) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        requestStr.append(buffer, bytes);
    }

    if (requestStr.empty()) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
        return;
    }

    std::istringstream reqStream(requestStr);
    std::string method, path, httpVersion;
    reqStream >> method >> path >> httpVersion;

    // Handle CORS preflight
    if (method == "OPTIONS") {
        sendResponse(clientSocket, 204, "No Content", "text/plain", "");
#if defined(_WIN32) || defined(_WIN64)
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
        return;
    }

    std::string lowerHeaders = requestStr.substr(0, (headerEnd != std::string::npos) ? headerEnd : requestStr.length());
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(), ::tolower);

    // Support Expect: 100-continue (e.g. PowerShell/.NET clients)
    if (lowerHeaders.find("expect: 100-continue") != std::string::npos) {
        const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
        send(clientSocket, cont, static_cast<int>(strlen(cont)), 0);
    }

    // Extract body
    std::string body = "";
    if (headerEnd != std::string::npos && headerEnd + 4 < requestStr.length()) {
        body = requestStr.substr(headerEnd + 4);
    }

    // Read full body if Content-Length specified
    size_t clPos = lowerHeaders.find("content-length:");
    if (clPos != std::string::npos) {
        size_t numStart = lowerHeaders.find_first_not_of(" \t", clPos + 15);
        size_t numEnd = lowerHeaders.find_first_of("\r\n", numStart);
        if (numStart != std::string::npos && numEnd != std::string::npos) {
            try {
                int contentLength = std::stoi(lowerHeaders.substr(numStart, numEnd - numStart));
                while (static_cast<int>(body.length()) < contentLength) {
                    int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                    if (bytes <= 0) break;
                    body.append(buffer, bytes);
                }
            } catch (...) {}
        }
    }

    // Route API calls
    if (path.find("/api/") == 0) {
        if (method == "GET" && path == "/api/flights") {
            std::string json = airlineManager->getAllFlightsJson();
            sendResponse(clientSocket, 200, "OK", "application/json", json);
        } else if (method == "GET" && path == "/api/perks") {
            std::string json = airlineManager->getClassPerksJson();
            sendResponse(clientSocket, 200, "OK", "application/json", json);
        } else if (method == "GET" && path.find("/api/flights/") == 0) {
            // Check for /api/flights/{id}/passengers
            std::string sub = path.substr(13); // after /api/flights/
            size_t slashPos = sub.find("/passengers");
            if (slashPos != std::string::npos) {
                std::string flightNo = sub.substr(0, slashPos);
                std::string json = airlineManager->getPassengersForFlightJson(flightNo);
                sendResponse(clientSocket, 200, "OK", "application/json", json);
            } else {
                std::string json = airlineManager->getFlightJson(sub);
                sendResponse(clientSocket, 200, "OK", "application/json", json);
            }
        } else if (method == "GET" && path == "/api/bookings") {
            std::string json = airlineManager->getAllBookingsJson();
            sendResponse(clientSocket, 200, "OK", "application/json", json);
        } else if (method == "GET" && path.find("/api/bookings/") == 0) {
            std::string ticketId = path.substr(14);
            std::string json = airlineManager->getBookingByTicketIdJson(ticketId);
            sendResponse(clientSocket, 200, "OK", "application/json", json);
        } else if (method == "GET" && path == "/api/stats") {
            std::string json = airlineManager->getSystemStatsJson();
            sendResponse(clientSocket, 200, "OK", "application/json", json);
        } else if (method == "POST" && path == "/api/book") {
            // Parse JSON fields
            std::string flightNo = JsonUtils::getString(body, "flightNumber");
            std::string fullName = JsonUtils::getString(body, "passengerName");
            std::string email = JsonUtils::getString(body, "email");
            std::string phone = JsonUtils::getString(body, "phone");
            std::string passport = JsonUtils::getString(body, "passport");
            int age = JsonUtils::getInt(body, "age", 25);
            std::string gender = JsonUtils::getString(body, "gender", "Other");
            std::string seatClass = JsonUtils::getString(body, "seatClass", "Economy");
            std::string requestedSeat = JsonUtils::getString(body, "requestedSeat", "");

            Passenger pass(fullName, email, phone, passport, age, gender);
            std::string resultJson = airlineManager->bookTicket(flightNo, pass, seatClass, requestedSeat);
            sendResponse(clientSocket, 200, "OK", "application/json", resultJson);
        } else if (method == "POST" && path == "/api/cancel") {
            std::string ticketId = JsonUtils::getString(body, "ticketId");
            std::string resultJson = airlineManager->cancelTicket(ticketId);
            sendResponse(clientSocket, 200, "OK", "application/json", resultJson);
        } else if (method == "POST" && path == "/api/reset") {
            airlineManager->resetData();
            sendResponse(clientSocket, 200, "OK", "application/json", "{\"success\": true, \"message\": \"System reset to default state.\"}");
        } else if (method == "GET" && (path == "/api/excel" || path == "/airline_bookings.xlsx")) {
            std::string content = readFileContent("data/airline_bookings.xlsx");
            if (!content.empty()) {
                sendResponse(clientSocket, 200, "OK", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", content);
            } else {
                sendResponse(clientSocket, 404, "Not Found", "application/json", "{\"error\": \"Excel file not found\"}");
            }
        } else {
            sendResponse(clientSocket, 404, "Not Found", "application/json", "{\"error\": \"API endpoint not found\"}");
        }
    } else {
        // Serve static files
        std::string filePath = "public";
        if (path == "/" || path.empty()) {
            filePath += "/index.html";
        } else if (path == "/airline_bookings.xlsx" || path == "/data/airline_bookings.xlsx") {
            filePath = "data/airline_bookings.xlsx";
        } else if (path == "/airline_bookings.xml" || path == "/data/airline_bookings.xml") {
            filePath = "data/airline_bookings.xml";
        } else {
            // Strip any query parameters
            size_t qPos = path.find('?');
            std::string cleanPath = (qPos != std::string::npos) ? path.substr(0, qPos) : path;
            filePath += cleanPath;
        }

        std::string content = readFileContent(filePath);
        if (!content.empty()) {
            std::string mime = getMimeType(filePath);
            sendResponse(clientSocket, 200, "OK", mime, content);
        } else {
            std::string notFound = "<h1>404 Not Found</h1><p>The requested file " + path + " was not found on this server.</p>";
            sendResponse(clientSocket, 404, "Not Found", "text/html", notFound);
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

void HttpServer::start() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    int wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaRes != 0) {
        std::cerr << "WSAStartup failed with error: " << wsaRes << std::endl;
        return;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
#if defined(_WIN32) || defined(_WIN64)
        WSACleanup();
#endif
        return;
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(port));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << std::endl;
#if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
#if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return;
    }

    isRunning = true;
    std::cout << "========================================================\n";
    std::cout << "  AIRLINE RESERVATION SYSTEM - PURE C++ BACKEND         \n";
    std::cout << "========================================================\n";
    std::cout << " [OK] Server started successfully!\n";
    std::cout << " [OK] Listening on: http://localhost:" << port << "\n";
    std::cout << " [OK] Web interface available in browser.\n";
    std::cout << " [OK] Press Ctrl+C in this console to stop the server.\n";
    std::cout << "========================================================\n" << std::endl;

    while (isRunning) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        socket_t clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (!isRunning) break;
            continue;
        }

        handleClient(clientSocket);
    }
}

void HttpServer::stop() {
    if (isRunning) {
        isRunning = false;
        if (serverSocket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(serverSocket);
            WSACleanup();
#else
            close(serverSocket);
#endif
            serverSocket = INVALID_SOCKET;
        }
    }
}
