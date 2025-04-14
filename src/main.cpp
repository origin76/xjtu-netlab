#include <iostream>
#include <multiserver.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // 用于彩色输出
#include "configparser.hpp"
#include "http_handler.hpp"


std::shared_ptr<StaticFileHandler> g_staticHandler;
std::shared_ptr<UploadHandler> g_uploadHandler;
std::unordered_map<std::string, std::shared_ptr<ProxyHandler>> g_proxyHandlers;

std::string g_uploadPathPrefix;


void handleRequest(const HttpRequest &request, HttpResponse &response) {
    spdlog::info("Received request: {} {}", request.getMethod(), request.getPath());
    const std::string &path = request.getPath();
    const std::string &method = request.getMethod();

    for (const auto &entry: g_proxyHandlers) {
        spdlog::info("Proxy checking: {}",entry.first);
        if (path.rfind(entry.first, 0) == 0) {
            entry.second->handle(request, response);
            return;
        }
    }
    if (path.rfind(g_uploadPathPrefix, 0) == 0) {
        g_uploadHandler->handle(request, response);
    } else if (method == "GET" || method == "POST" || method == "HEAD") {
        g_staticHandler->handle(request, response);
    } else {
        response.setStatus(405, "Method Not Allowed");
        response.setBody("Unsupported method");
    }
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
        auto proxyConfig = ConfigCenter::instance().getProxyConfig();
        auto uploadConfig = ConfigCenter::instance().getUploadConfig();

        g_staticHandler = std::make_shared<StaticFileHandler>(siteConfig->getRootDirectory());
        g_uploadPathPrefix = uploadConfig->getRequestPath(); // 例如 "/upload"
        std::string uploadStoragePath = uploadConfig->getStoragePath(); // 例如 "./uploads"
        g_uploadHandler = std::make_shared<UploadHandler>(uploadStoragePath);
        g_proxyHandlers = proxyConfig->getProxyMap();


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
