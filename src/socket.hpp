#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <address.hpp>

class Socket : public std::enable_shared_from_this<Socket> {
public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    Socket(int family, int type, int protocol = 0)
        : m_family(family), m_type(type), m_protocol(protocol), m_isConnected(false) {
        newSock();
        initSock();
    }

    explicit Socket(int sockfd)
        : std::enable_shared_from_this<Socket>(),
          m_sockfd(sockfd), m_family(0), m_type(0), m_protocol(0), m_isConnected(false) {
        initSock();
    }

    virtual ~Socket() {
        if (m_sockfd != -1) {
            ::close(m_sockfd);
        }
        if (ssl) {
            SSL_free(ssl);
        }
        if (ctx) {
            SSL_CTX_free(ctx);
        }
    }

    static ptr CreateTCP(Address::ptr address) {
        return ptr(new Socket(address->getFamily(), SOCK_STREAM, 0));
    }

    static ptr CreateTCPSocket() {
        return ptr(new Socket(AF_INET, SOCK_STREAM, 0));
    }

    static ptr CreateSSL(Address::ptr address) {
        ptr server = ptr(new Socket(address->getFamily(), SOCK_STREAM, 0));
        server->initSSL();
        return server;
    }

    bool bind(Address::ptr address) {
        if (::bind(m_sockfd, address->getAddress(), address->getLength()) == -1) {
            return false;
        }
        m_localAddress = address;
        return true;
    }

    bool listen(int backlog = SOMAXCONN) {
        if (::listen(m_sockfd, backlog) == -1) {
            return false;
        }
        return true;
    }

    ptr accept() {
        struct sockaddr_storage addr;
        socklen_t len = sizeof(addr);
        int sock = ::accept(m_sockfd, reinterpret_cast<struct sockaddr*>(&addr), &len);
        if (sock == -1) {
            return nullptr;
        }

        ptr client(new Socket(sock));
        client->m_remoteAddress = Address::ptr(new IPv4Address(*reinterpret_cast<struct sockaddr_in*>(&addr)));
        client->m_localAddress = Address::getLocalAddress(sock);
        client->m_isConnected = true;

        if (ssl) {
            client->ssl = SSL_new(ctx);
            SSL_set_fd(client->ssl, sock);
            if (SSL_accept(client->ssl) <= 0) {
                SSL_free(client->ssl);
                client->ssl = nullptr;
                ::close(sock);
                return nullptr;
            }
        }

        return client;
    }

    bool send(const void* buffer, size_t length) {
        if (ssl) {
            int bytes_sent = SSL_write(ssl, buffer, length);
            return bytes_sent != -1 && static_cast<size_t>(bytes_sent) == length;
        } else {
            ssize_t bytes_sent = ::send(m_sockfd, buffer, length, 0);
            return bytes_sent != -1 && static_cast<size_t>(bytes_sent) == length;
        }
    }

    bool recv(void* buffer, size_t length, size_t* received = nullptr) {
        if (ssl) {
            int bytes_received = SSL_read(ssl, buffer, length);
            if (bytes_received == -1) {
                return false;
            }
            if (received) {
                *received = static_cast<size_t>(bytes_received);
            }
            return true;
        } else {
            ssize_t bytes_received = ::recv(m_sockfd, buffer, length, 0);
            if (bytes_received == -1) {
                return false;
            }
            if (received) {
                *received = static_cast<size_t>(bytes_received);
            }
            return true;
        }
    }

    int getSocket() const {
        return m_sockfd;
    }

    Address::ptr getLocalAddress() const {
        return m_localAddress;
    }

    Address::ptr getRemoteAddress() const {
        return m_remoteAddress;
    }

    bool isConnected() const {
        return m_isConnected;
    }

    void setSSL(SSL_CTX* ctx) {
        this->ctx = ctx;
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, m_sockfd);
    }

private:
    void newSock() {
        m_sockfd = socket(m_family, m_type, m_protocol);
        if (m_sockfd == -1) {
            throw std::runtime_error("Failed to create socket");
        }
    }

    void initSock() {
        int val = 1;
        setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    }

    bool initSSL() {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        ERR_load_BIO_strings();

        ctx = SSL_CTX_new(SSLv23_server_method());
        if (!ctx) {
            return false;
        }

        if (SSL_CTX_use_certificate_file(ctx, "../cert.pem", SSL_FILETYPE_PEM) <= 0) {
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, "../key.pem", SSL_FILETYPE_PEM) <= 0) {
            return false;
        }

        setSSL(ctx);

        return true;
    }

    void close() {
        if (m_sockfd != -1) {
            ::close(m_sockfd);
            m_sockfd = -1;
        }
    }

    int m_sockfd = -1;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
};