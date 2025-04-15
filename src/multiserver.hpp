#pragma once
#include <cmath>
#include <spdlog/spdlog.h>

#include "server.hpp"
#include "threadpool.hpp"

class MultiThreadedHttpServer {
public:
    MultiThreadedHttpServer(Socket::ptr sock, size_t num_threads, bool keep_alive = true) :
        m_sock(sock), m_isRunning(false), m_keepAlive(keep_alive), m_threadPool(num_threads) {}

    bool start() {
        m_isRunning = true;
        m_acceptThread = std::thread(&MultiThreadedHttpServer::acceptLoop, this);
        return true;
    }

    void stop() {
        m_isRunning = false;
        if (m_acceptThread.joinable()) {
            m_acceptThread.join();
        }
        m_threadPool.~ThreadPool();
    }

    void setHandle(HttpCallback cb) { m_handle = cb; }

private:
    void acceptLoop() {
        while (m_isRunning) {
            Socket::ptr client = m_sock->accept();
            if (client) {
                m_threadPool.enqueue([this, client]() { handleRequest(client); });
            }
        }
    }

    void handleRequest(Socket::ptr client) {
        while (m_isRunning) {
            HttpRequest request;
            HttpResponse response(client);

            if (!request.parse(client)) {
                response.setStatus(400, "Bad Request");
                response.send();
                break;
            }

            spdlog::info("[socket] Request Addr is {}", client->getRemoteAddress()->toString());

            bool keepAlive = true;
            std::string connHeader = request.getHeader("Connection");
            std::transform(connHeader.begin(), connHeader.end(), connHeader.begin(), ::tolower);
            if (connHeader == "close") {
                keepAlive = false;
            } else if (connHeader.empty()) {
                // HTTP/1.1 默认 keep-alive，但 HTTP/1.0 默认是 close
                keepAlive = (request.getVersion() == "HTTP/1.1");
            }

            if (keepAlive){
                if (!client->enableKeepAlive()) {
                    std::cerr << "Failed to enable Keep-Alive" << std::endl;
                }
            }

            if (m_handle) {
                m_handle(request, response);
            } else {
                response.setStatus(404, "Not Found");
            }

            response.setHeader("Connection", m_keepAlive ? "keep-alive" : "close");
            response.send();

            if (keepAlive) {
                while (true) {
                    spdlog::info("ip {} keepalive once",client->getRemoteAddress()->toString());
                    if (!client->sendHeartbeat()) {
                        break;
                    }

                    HttpRequest request;
                    HttpResponse response(client);

                    request.parse(client);
                    if (!request.parse(client)) {
                        break;
                    }
                    if (request.getHeader("Connection") != "Keep-Alive") {
                        break;
                    }
                    if (m_handle) {
                        m_handle(request, response);
                    } else {
                        response.setStatus(404, "Not Found");
                    }
                    response.send();
                }
            }
        }
    }


    Socket::ptr m_sock;
    bool m_isRunning;
    bool m_keepAlive;
    std::thread m_acceptThread;
    ThreadPool m_threadPool;
    HttpCallback m_handle;
};
