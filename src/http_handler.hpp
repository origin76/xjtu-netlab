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
    explicit StaticFileHandler(const std::string &root);

    void handle(const HttpRequest &req, HttpResponse &res) override;

private:
    std::string m_rootPath;
    std::shared_ptr<FileCacheManager> m_cache;
    std::string getMimeType(const std::string &path);
};


StaticFileHandler::StaticFileHandler(const std::string &root) :
    m_rootPath(root), m_cache(std::make_shared<FileCacheManager>()) {
}

void StaticFileHandler::handle(const HttpRequest &req, HttpResponse &res) {
    if (req.getMethod() != "GET" && req.getMethod() != "HEAD") {
        res.setStatus(405, "Method Not Allowed");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Method Not Allowed");
        return;
    }

    std::string relPath = req.getPath() == "/" ? "/index.html" : req.getPath();
    std::string fullPath = m_rootPath + relPath;

    if (!std::filesystem::exists(fullPath) || std::filesystem::is_directory(fullPath)) {
        res.setStatus(404, "Not Found");
        res.setHeader("Content-Type", "text/plain");
        res.setBody("File not found");
        return;
    }

    std::optional<std::string> cached = m_cache->get(fullPath);
    std::string content;

    if (cached) {
        content = *cached;
    } else {
        std::ifstream ifs(fullPath, std::ios::binary);
        if (!ifs) {
            res.setStatus(500, "Internal Server Error");
            res.setBody("Failed to read file");
            return;
        }

        std::ostringstream oss;
        oss << ifs.rdbuf();
        content = oss.str();
        m_cache->put(fullPath, content);
    }

    res.setStatus(200, "OK");
    res.setHeader("Content-Type", getMimeType(fullPath));
    res.setHeader("Content-Length", std::to_string(content.size()));
    if (req.getMethod() != "HEAD") {
        res.setBody(content);
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
