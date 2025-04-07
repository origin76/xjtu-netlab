#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <socket.hpp>
#include <map>

class HttpRequest {
    public:
        bool parse(Socket::ptr sock) {
            char buffer[4096];
            size_t received = 0;
            if (!sock->recv(buffer, sizeof(buffer), &received)) {
                return false;
            }
    
            std::string data(buffer, received);
            std::istringstream iss(data);
    
            std::string method, path, version;
            iss >> method >> path >> version;
    
            m_method = method;
            m_path = path;
            m_version = version;
    
            // 解析头部
            std::string line;
            while (std::getline(iss, line) && line != "\r") {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    m_headers[key] = value;
                }
            }
    
            // 解析主体
            m_body = std::string(std::istreambuf_iterator<char>(iss), std::istreambuf_iterator<char>());
    
            return true;
        }
    
        const std::string& getMethod() const {
            return m_method;
        }
    
        const std::string& getPath() const {
            return m_path;
        }
    
        const std::string& getVersion() const {
            return m_version;
        }
    
        const std::map<std::string, std::string>& getHeaders() const {
            return m_headers;
        }
    
        const std::string& getHeader(const std::string& key) const {
            auto it = m_headers.find(key);
            if (it != m_headers.end()) {
                return it->second;
            }
            return empty_string;
        }
    
        const std::string& getBody() const {
            return m_body;
        }
    
    private:
        std::string m_method;
        std::string m_path;
        std::string m_version;
        std::map<std::string, std::string> m_headers;
        std::string m_body;
        static const std::string empty_string;
    };
    
    const std::string HttpRequest::empty_string;
    
    class HttpResponse {
    public:
        HttpResponse(Socket::ptr sock) : m_sock(sock) {}
    
        void setStatus(int status, const std::string& reason) {
            m_status = status;
            m_reason = reason;
        }
    
        void setHeader(const std::string& key, const std::string& value) {
            m_headers[key] = value;
        }
    
        void setBody(const std::string& body) {
            m_body = body;
        }
    
        void send() {
            std::ostringstream oss;
            oss << "HTTP/1.1 " << m_status << " " << m_reason << "\r\n";
    
            for (const auto& header : m_headers) {
                oss << header.first << ": " << header.second << "\r\n";
            }
    
            oss << "Content-Length: " << m_body.size() << "\r\n";
            oss << "\r\n";
            oss << m_body;
    
            std::string response = oss.str();
            m_sock->send(response.c_str(), response.size());
        }
    
    private:
        Socket::ptr m_sock;
        int m_status = 200;
        std::string m_reason = "OK";
        std::map<std::string, std::string> m_headers;
        std::string m_body;
    };

using HttpCallback = std::function<void(const HttpRequest&, HttpResponse&)>;

class HttpServer {
public:
    HttpServer(Socket::ptr sock, bool keep_alive = true)
        : m_sock(sock), m_isRunning(false), m_keepAlive(keep_alive) {}

    bool start() {
        m_isRunning = true;
        m_thread = std::thread(&HttpServer::run, this);
        return true;
    }

    void stop() {
        m_isRunning = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void setHandle(HttpCallback cb) {
        m_handle = cb;
    }

private:
    void run() {
        while (m_isRunning) {
            Socket::ptr client = m_sock->accept();
            if (client) {
                handleRequest(client);
            }
        }
    }

    void handleRequest(Socket::ptr client) {
        HttpRequest request;
        HttpResponse response(client);

        if (!request.parse(client)) {
            response.setStatus(400, "Bad Request");
            response.send();
            return;
        }

        if (m_handle) {
            m_handle(request, response);
        } else {
            response.setStatus(404, "Not Found");
        }

        response.send();
    }

    Socket::ptr m_sock;
    bool m_isRunning;
    bool m_keepAlive;
    std::thread m_thread;
    HttpCallback m_handle;
};
