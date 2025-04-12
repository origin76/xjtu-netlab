#pragma once
#include "server.hpp"
#include "threadpool.hpp"

class MultiThreadedHttpServer
{
public:
    MultiThreadedHttpServer(Socket::ptr sock, size_t num_threads, bool keep_alive = true)
        : m_sock(sock), m_isRunning(false), m_keepAlive(keep_alive), m_threadPool(num_threads) {}

    bool start()
    {
        m_isRunning = true;
        m_acceptThread = std::thread(&MultiThreadedHttpServer::acceptLoop, this);
        return true;
    }

    void stop()
    {
        m_isRunning = false;
        if (m_acceptThread.joinable())
        {
            m_acceptThread.join();
        }
        m_threadPool.~ThreadPool();
    }

    void setHandle(HttpCallback cb)
    {
        m_handle = cb;
    }

private:
    void acceptLoop()
    {
        while (m_isRunning)
        {
            Socket::ptr client = m_sock->accept();
            if (client)
            {
                m_threadPool.enqueue([this, client]()
                                     { handleRequest(client); });
            }
        }
    }

    void handleRequest(Socket::ptr client)
    {
        HttpRequest request;
        HttpResponse response(client);

        if (!request.parse(client))
        {   
            response.setStatus(400, "Bad Request");
            response.send();
            return;
        }

        if (m_handle)
        {
            m_handle(request, response);
        }
        else
        {
            response.setStatus(404, "Not Found");
        }

        response.send();
    }

    Socket::ptr m_sock;
    bool m_isRunning;
    bool m_keepAlive;
    std::thread m_acceptThread;
    ThreadPool m_threadPool;
    HttpCallback m_handle;
};