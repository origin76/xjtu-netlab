#include <iostream>
#include <multiserver.hpp>

void handleRequest(const HttpRequest& request, HttpResponse& response) {
    std::cout << "Received request: " << request.getMethod() << " " << request.getPath() << std::endl;

    if (request.getMethod() == "GET" && request.getPath() == "/") {
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Hello, World!");
    } else {
        response.setStatus(404, "Not Found");
        response.setBody("Page not found");
    }
}

int main() {
    try {
        auto address = Address::createIPv4Address(8080, "0.0.0.0");
        auto sock = Socket::CreateTCP(address);
        sock->bind(address);
        sock->listen();

        MultiThreadedHttpServer server(sock, 4); // 使用 4 个线程
        server.setHandle(handleRequest);
        server.start();

        std::cout << "Server started on port 8080" << std::endl;
        std::cin.get();
        server.stop();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}