#pragma once
#include <optional>
#include "server.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <spdlog/spdlog.h>


class FileCacheManager {
public:
    std::optional<std::string> get(const std::string &path);
    void put(const std::string &path, const std::string &content);
    void clear();

private:
    std::unordered_map<std::string, std::string> m_cache;
};

class HttpHandler {
public:
    virtual void handle(const HttpRequest &req, HttpResponse &res) = 0;
    virtual ~HttpHandler() = default;
};

class StaticFileHandler : public HttpHandler {
public:
    StaticFileHandler(const std::string &root, const std::string &defaultSite);

    std::vector<std::string> splitMultipartBody(const std::string &body, const std::string &boundary);

    std::vector<std::string> splitString(const std::string &str, const std::string &delimiter);

    void handlePostRequest(const HttpRequest &req, HttpResponse &res);

    void handle(const HttpRequest &req, HttpResponse &res) override;

private:
    std::string m_rootPath;
    std::string m_defaultSite;
    std::shared_ptr<FileCacheManager> m_cache;
    std::string getMimeType(const std::string &path);
};


StaticFileHandler::StaticFileHandler(const std::string &root, const std::string &defaultSite) :
    m_rootPath(root), m_cache(std::make_shared<FileCacheManager>()) {
    if (defaultSite.empty() || defaultSite == "/") {
        m_defaultSite = "/index.html";
    } else if (defaultSite.starts_with("./")) {
        m_defaultSite = "/" + defaultSite.substr(2);
    } else if (!defaultSite.starts_with("/")) {
        m_defaultSite = "/" + defaultSite;
    } else {
        m_defaultSite = defaultSite;
    }
    if (!m_rootPath.empty() && m_rootPath.back() == '/') {
        m_rootPath.pop_back();
    }
}

void StaticFileHandler::handle(const HttpRequest &req, HttpResponse &res) {
    spdlog::info("[StaticFileHandler] Handling request: {} {}", req.getMethod(), req.getPath());

    if (req.getMethod() != "GET" && req.getMethod() != "HEAD") {
        spdlog::info("[StaticFileHandler] Method not allowed: {}", req.getMethod());
        res.setStatus(405, "Method Not Allowed");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Method Not Allowed");
        return;
    }

    // 计算请求的文件路径
    std::string relPath = req.getPath() == "/" ? m_defaultSite : req.getPath();
    std::string fullPath = m_rootPath + relPath;

    spdlog::info("[StaticFileHandler] Requested file path: {}", fullPath);

    // 检查文件是否存在
    if (!std::filesystem::exists(fullPath) || std::filesystem::is_directory(fullPath)) {
        spdlog::warn("[StaticFileHandler] File not found or it's a directory: {}", fullPath);
        res.setStatus(404, "Not Found");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("File not found");
        return;
    }

    // 尝试获取缓存
    std::optional<std::string> cached = m_cache->get(fullPath);
    std::string content;
    res.m_path = fullPath;

    if (cached) {
        spdlog::info("[StaticFileHandler] File found in cache: {}", fullPath);
        content = *cached;
    } else {
        spdlog::info("[StaticFileHandler] File not in cache, reading from disk: {}", fullPath);

        // 读取文件内容
        std::ifstream ifs(fullPath, std::ios::binary);
        if (!ifs) {
            spdlog::error("[StaticFileHandler] Failed to read file: {}", fullPath);
            res.setStatus(500, "Internal Server Error");
            res.setBody("Failed to read file");
            return;
        }

        std::ostringstream oss;
        oss << ifs.rdbuf();
        content = oss.str();

        // 缓存文件内容
        m_cache->put(fullPath, content);
        spdlog::info("[StaticFileHandler] File read from disk and cached: {}", fullPath);
    }

    // 设置响应
    res.setStatus(200, "OK");
    res.setHeader("Content-Type", getMimeType(fullPath));
    // res.setHeader("Content-Length", std::to_string(content.size()));

    if (req.getMethod() != "HEAD") {
        spdlog::info("[StaticFileHandler] Setting response body.");
        res.setBody(content);
    } else {
        spdlog::info("[StaticFileHandler] HEAD request, no response body set.");
    }
}

std::string StaticFileHandler::getMimeType(const std::string &path) {
    static const std::unordered_map<std::string, std::string> mimeTypes = {
            {".html", "text/html"}, {".htm", "text/html"}, {".css", "text/css"},
            {".js", "application/javascript"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"}, {".gif", "image/gif"}, {".ico", "image/x-icon"},
            {".svg", "image/svg+xml"}, {".json", "application/json"}, {".txt", "text/plain"}
    };

    std::filesystem::path ext = std::filesystem::path(path).extension();
    auto it = mimeTypes.find(ext.string());
    return it != mimeTypes.end() ? it->second : "application/octet-stream";
}

std::optional<std::string> FileCacheManager::get(const std::string &path) {
    auto it = m_cache.find(path);
    return it != m_cache.end() ? std::optional(it->second) : std::nullopt;
}

void FileCacheManager::put(const std::string &path, const std::string &content) {
    m_cache[path] = content;
}

void FileCacheManager::clear() {
    m_cache.clear();
}


class ProxyHandler : public HttpHandler {
public:
    explicit ProxyHandler(const std::string &targetUrl) :
        m_targetUrl(targetUrl) {
    }

    void handle(const HttpRequest &req, HttpResponse &res) override {
        // TODO: 实现请求转发，类似 Nginx 的 proxy_pass
        spdlog::info("Forwarding request to {}", m_targetUrl);

        // TODO: 使用类似 HTTP 客户端的逻辑去向目标地址转发请求并返回响应，这里可以使用 boost::asio 进行请求转发
        res.setStatus(502, "Bad Gateway");
        res.setBody("Proxy forwarding not implemented yet.");
    }

private:
    std::string m_targetUrl;
};


class UploadHandler : public HttpHandler {
public:
    UploadHandler(const std::string &uploadPath) :
        m_uploadPath(uploadPath) {
        std::filesystem::create_directories(m_uploadPath);
    }

    void handle(const HttpRequest &req, HttpResponse &res) override {
        if (req.getMethod() != "POST") {
            res.setStatus(405, "Method Not Allowed");
            res.setBody("Only POST method is allowed for upload.");
            return;
        }

        std::string filename = "upload_" + std::to_string(std::time(nullptr)) + ".bin";
        std::string fullPath = m_uploadPath + "/" + filename;

        std::ofstream ofs(fullPath, std::ios::binary);
        if (!ofs) {
            res.setStatus(500, "Internal Server Error");
            res.setBody("Failed to save uploaded file.");
            return;
        }

        ofs << req.getBody();
        res.setStatus(200, "OK");
        res.setBody("File uploaded successfully as " + filename);
    }

private:
    std::string m_uploadPath;
};
