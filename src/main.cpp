#include <iostream>
#include <multiserver.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // 用于彩色输出
#include "configparser.hpp"
#include "http_handler.hpp"


std::shared_ptr<StaticFileHandler> g_staticHandler;

void handleRequest(const HttpRequest &request, HttpResponse &response) {
    spdlog::info("Received request: {} {}", request.getMethod(), request.getPath());
    g_staticHandler->handle(request, response);
}

int main() {
    try {
        spdlog::set_level(spdlog::level::info);
        auto console = spdlog::stdout_color_mt("console");
        console->set_pattern("%^[%l] %v%$");
        spdlog::info("Starting the application...");


        ConfigCenter::instance().init("config.ini");
        auto serverConfig = ConfigCenter::instance().getServerConfig();
        auto siteConfig = ConfigCenter::instance().getSiteConfig();

        g_staticHandler = std::make_shared<StaticFileHandler>(siteConfig->getRootDirectory());

        auto address = Address::createIPv4Address(serverConfig->getPort(), serverConfig->getAllowedIps());
        auto sock = Socket::CreateSSL(address);
        sock->bind(address);
        sock->listen();

        MultiThreadedHttpServer server(sock, 4); // 使用 4 个线程
        server.setHandle(handleRequest);
        server.start();

        spdlog::info("Server started on port {}", serverConfig->getPort());

        std::cin.get();
        server.stop();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
