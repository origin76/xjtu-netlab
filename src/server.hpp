#pragma once

#include <boost/url.hpp>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <socket.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <gzip.hpp>
// #include <boost/url/src.hpp>

class HttpRequest {
public:
    bool parse(Socket::ptr sock) {
        char buffer[100000000]; // 使用较大的缓冲区
        size_t total_received = 0;
        std::string data;

        // 循环接收数据，直到接收到完整的请求
        while (true) {
            spdlog::info("[HttpRequest] parsing loop ...");
            size_t received = 0;
            if (!sock->recv(buffer, sizeof(buffer), &received)) {
                spdlog::info("[HttpRequest] parsing failed");
                return false;
            }
            total_received += received;
            data.append(buffer, received);

            // 检查是否接收到完整的请求
            if (isRequestComplete(data)) {
                break;
            }
        }

        std::istringstream iss(data);

        // 解析请求行
        std::string method, path, version;
        iss >> method >> path >> version;

        m_method = method;
        m_path = path;
        m_version = version;

        // 解析主体
        m_body = std::string(std::istreambuf_iterator<char>(iss), std::istreambuf_iterator<char>());


        spdlog::info("[HttpRequest] parsing successful");
        return true;
    }

    const std::string getMethod() const { return m_method; }

    const std::string getPath() const { return m_path; }

    const std::string getVersion() const { return m_version; }

    const std::map<std::string, std::string> getHeaders() const { return m_headers; }

    const std::string &getHeader(const std::string &key) const {
        auto it = m_headers.find(key);
        if (it != m_headers.end()) {
            return it->second;
        }
        return empty_string;
    }

    const bool hasHeader(const std::string &key) const {
        auto it = m_headers.find(key);
        if (it != m_headers.end()) {
            return true;
        }
        return false;
    }

    const std::string &getBody() const { return m_body; }

private:
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
    static const std::string empty_string;

    bool isRequestComplete(const std::string &data) {
        // 检查是否包含两个连续的 CRLF，表示头部结束
        spdlog::info("[HttpRequest] Checking Request header...");
        size_t crlfPos = data.find("\r\n\r\n");
        if (crlfPos == std::string::npos) {
            return false;
        }
        spdlog::info("[HttpRequest] Request header accepted, waiting for body ...");

        // 提取头部部分
        std::string headers = data.substr(0, crlfPos);

        // 解析 Content-Length
        size_t contentLength = 0;
        std::istringstream iss(headers);
        std::string line;
        while (std::getline(iss, line) && line != "\r") {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                m_headers[key] = value;
                if (key == "Content-Length") {
                    contentLength = std::stoull(value);
                }
            }
        }


        spdlog::info("[HttpRequest] Body acquired");
        // 检查请求体长度是否符合 Content-Length
        size_t bodyLength = data.size() - crlfPos - 4; // 减去 "\r\n\r\n" 的长度
        if (bodyLength >= contentLength) {
            spdlog::info("[HttpRequest] Request finished");
            return true;
        }
        spdlog::info("[HttpRequest] Request not finish...");

        return false;
    }
};

bool saveStringToFile(const std::string &content, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    file.write(content.data(), content.size());
    if (!file) {
        std::cerr << "Failed to write to file: " << filename << std::endl;
        return false;
    }

    file.close();
    return true;
}

const std::string HttpRequest::empty_string;

class HttpResponse {
public:
    HttpResponse(Socket::ptr sock) :
        m_sock(sock) {
    }

    void setStatus(int status, const std::string &reason) {
        m_status = status;
        m_reason = reason;
    }

    void setHeader(const std::string &key, const std::string &value) { m_headers[key] = value; }

    void setBody(const std::string &body) { m_body = body; }

    void send() {
        // 检查是否需要压缩
        if (m_headers.find("Content-Encoding") != m_headers.end() && m_headers["Content-Encoding"] == "gzip") {
            string compressedBody;
            if (!GzipHandler::compress(m_body, compressedBody)) {
                sendResponse();
                return;
            }
            m_body = compressedBody;
        }

        sendResponse();
    }

private:
    Socket::ptr m_sock;
    int m_status = 200;
    std::string m_reason = "OK";
    std::map<std::string, std::string> m_headers;
    std::string m_body;

    void sendResponse() {
        ostringstream oss;
        oss << "HTTP/1.1 " << m_status << " " << m_reason << "\r\n";

        for (const auto &header: m_headers) {
            oss << header.first << ": " << header.second << "\r\n";
        }

        oss << "Content-Length: " << m_body.size() << "\r\n";
        oss << "\r\n";
        oss << m_body;

        string response = oss.str();
        saveStringToFile(response, "output.txt");
        m_sock->send(response.c_str(), response.size());
    }
};

using HttpCallback = std::function<void(const HttpRequest &, HttpResponse &)>;
