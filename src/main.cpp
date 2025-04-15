#include <iostream>
#include <multiserver.hpp>
#include <spdlog/sinks/stdout_color_sinks.h> // 鐢ㄤ簬褰╄壊杈撳嚭
#include <spdlog/spdlog.h>
#include "configparser.hpp"
#include "http_handler.hpp"
#include "cgi.hpp"


std::shared_ptr<StaticFileHandler> g_staticHandler;
std::shared_ptr<UploadHandler> g_uploadHandler;
std::shared_ptr<CGIHandler> g_cgiHandler;
std::unordered_map<std::string, std::shared_ptr<ProxyHandler>> g_proxyHandlers;

std::string g_uploadPathPrefix;


void handleRequest(const HttpRequest &request, HttpResponse &response) {
    spdlog::info("[handleRequest] Received request: {} {}", request.getMethod(), request.getPath());

    const std::string &path = request.getPath();
    const std::string &method = request.getMethod();

    // 代理检查
    for (const auto &entry: g_proxyHandlers) {
        spdlog::debug("[handleRequest] Proxy checking: prefix = {}", entry.first);
        if (path.rfind(entry.first, 0) == 0) {
            if (!entry.second) {
                spdlog::error("[handleRequest] Proxy handler for '{}' is null!", entry.first);
                response.setStatus(500, "Internal Server Error");
                response.setBody("Proxy handler not available");
                return;
            }
            spdlog::debug("[handleRequest] Matched proxy, handling with ProxyHandler...");
            entry.second->handle(request, response);
            return;
        }
    }

    if (path.find("/cgi/") == 0) {
        g_cgiHandler->handle(request, response);
        return;
    }

    // 上传处理
    if (path.rfind(g_uploadPathPrefix, 0) == 0) {
        spdlog::info("[handleRequest] Upload path matched, using UploadHandler...");
        if (!g_uploadHandler) {
            spdlog::error("[handleRequest] Upload handler is null!");
            response.setStatus(500, "Internal Server Error");
            response.setBody("Upload handler not available");
            return;
        }
        g_uploadHandler->handle(request, response);
        return;
    }

    // 静态资源处理
    if (method == "GET" || method == "POST" || method == "HEAD") {
        spdlog::info("[handleRequest] Handling with StaticFileHandler...");
        if (!g_staticHandler) {
            spdlog::error("[handleRequest] Static file handler is null!");
            response.setStatus(500, "Internal Server Error");
            response.setBody("Static handler not available");
            return;
        }
        g_staticHandler->handle(request, response);
        return;
    }

    // 方法不支持
    spdlog::warn("[handleRequest] Unsupported method: {} {}", method , method.length());
    response.setStatus(405, "Method Not Allowed");
    response.setBody("Unsupported method");
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

        g_staticHandler = std::make_shared<StaticFileHandler>(siteConfig->getRootDirectory(), siteConfig->getDefaultSite());
        g_uploadPathPrefix = uploadConfig->getRequestPath(); // 例如 "/upload"
        g_cgiHandler = std::make_shared<CGIHandler>(siteConfig->getRootDirectory());
        std::string uploadStoragePath = uploadConfig->getStoragePath(); // 例如 "./uploads"
        g_uploadHandler = std::make_shared<UploadHandler>(uploadStoragePath);
        g_proxyHandlers = proxyConfig->getProxyMap();


        auto address = Address::createIPv4Address(serverConfig->getPort(), serverConfig->getAllowedIps());
        auto sock = Socket::CreateSSL(address);
        sock->bind(address);
        sock->listen();

        MultiThreadedHttpServer server(sock, serverConfig->getThreads()); // 使用 4 个线程
        server.setHandle(handleRequest);
        server.start();

        spdlog::info("Server started on port {}", serverConfig->getPort());
        ConfigCenter::instance().printConfigInfo();

        std::cin.get();
        server.stop();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
