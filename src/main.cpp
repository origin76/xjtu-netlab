#include <iostream>
#include <multiserver.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // 用于彩色输出


void handleRequest(const HttpRequest &request, HttpResponse &response) {
    spdlog::info("Received request: {} {}", request.getMethod(), request.getPath());
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
        spdlog::set_level(spdlog::level::info);
        auto console = spdlog::stdout_color_mt("console");
        console->set_pattern("%^[%l] %v%$");
        spdlog::info("Starting the application...");

        uint16_t port = 8080;

        auto address = Address::createIPv4Address(port, "0.0.0.0");
        auto sock = Socket::CreateSSL(address);
        sock->bind(address);
        sock->listen();

        MultiThreadedHttpServer server(sock, 4); // 使用 4 个线程
        server.setHandle(handleRequest);
        server.start();

        spdlog::info("Server started on port {}", port);

        std::cin.get();
        server.stop();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
