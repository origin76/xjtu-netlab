#include <memory>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <address.hpp>

class Socket : public std::enable_shared_from_this<Socket>
{
public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    static ptr CreateTCP(Address::ptr address)
    {
        return ptr(new Socket(address->getFamily(), SOCK_STREAM, 0));
    }

    static ptr CreateTCPSocket()
    {
        return ptr(new Socket(AF_INET, SOCK_STREAM, 0));
    }

    Socket(int family, int type, int protocol = 0)
        : std::enable_shared_from_this<Socket>(),
        m_family(family), m_type(type), m_protocol(protocol), m_isConnected(false)
    {
        newSock();
        initSock();
    }

    explicit Socket(int sockfd)
        : std::enable_shared_from_this<Socket>(),
          m_sockfd(sockfd), m_family(0), m_type(0), m_protocol(0), m_isConnected(false) {
        initSock();
    }

    Socket(const Socket& other)
        : std::enable_shared_from_this<Socket>(), 
          m_family(other.m_family),
          m_type(other.m_type),
          m_protocol(other.m_protocol),
          m_isConnected(other.m_isConnected),
          m_localAddress(other.m_localAddress),
          m_remoteAddress(other.m_remoteAddress) {
        ptr this_ptr = shared_from_this();
        newSock();
        initSock();
        if (other.m_isConnected) {
            m_sockfd = dup(other.m_sockfd);
            if (m_sockfd == -1) {
                throw std::runtime_error("Failed to duplicate socket");
            }
        }
    }

    virtual ~Socket()
    {
        if (m_sockfd != -1)
        {
            ::close(m_sockfd);
        }
    }

    bool bind(Address::ptr address)
    {
        if (::bind(m_sockfd, address->getAddress(), address->getLength()) == -1)
        {
            return false;
        }
        m_localAddress = address;
        return true;
    }

    bool listen(int backlog = SOMAXCONN)
    {
        if (::listen(m_sockfd, backlog) == -1)
        {
            return false;
        }
        return true;
    }

    ptr accept()
    {
        struct sockaddr_storage addr;
        socklen_t len = sizeof(addr);
        int sock = ::accept(m_sockfd, reinterpret_cast<struct sockaddr *>(&addr), &len);
        if (sock == -1)
        {
            return nullptr;
        }

        ptr sock_ptr(new Socket(sock));
        sock_ptr->m_remoteAddress = Address::ptr(new IPv4Address(*reinterpret_cast<struct sockaddr_in *>(&addr)));
        sock_ptr->m_localAddress = Address::getLocalAddress(sock);
        sock_ptr->m_isConnected = true;
        return sock_ptr;
    }

    bool connect(Address::ptr address)
    {
        if (::connect(m_sockfd, address->getAddress(), address->getLength()) == -1)
        {
            return false;
        }
        m_remoteAddress = address;
        m_localAddress = Address::getLocalAddress(m_sockfd);
        m_isConnected = true;
        return true;
    }

    bool send(const void *buffer, size_t length)
    {
        ssize_t bytes_sent = ::send(m_sockfd, buffer, length, 0);
        return bytes_sent != -1 && static_cast<size_t>(bytes_sent) == length;
    }

    bool recv(void *buffer, size_t length, size_t *received = nullptr)
    {
        ssize_t bytes_received = ::recv(m_sockfd, buffer, length, 0);
        if (bytes_received == -1)
        {
            return false;
        }
        if (received)
        {
            *received = static_cast<size_t>(bytes_received);
        }
        return true;
    }

    int getSocket() const
    {
        return m_sockfd;
    }

    Address::ptr getLocalAddress() const
    {
        return m_localAddress;
    }

    Address::ptr getRemoteAddress() const
    {
        return m_remoteAddress;
    }

    bool isConnected() const
    {
        return m_isConnected;
    }

protected:
    void newSock()
    {
        m_sockfd = socket(m_family, m_type, m_protocol);
        if (m_sockfd == -1)
        {
            throw std::runtime_error("Failed to create socket");
        }
    }

    void initSock()
    {
        int val = 1;
        setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    }

    bool init(int sock)
    {
        close();
        m_sockfd = sock;
        initSock();
        return true;
    }

    void close()
    {
        if (m_sockfd != -1)
        {
            ::close(m_sockfd);
            m_sockfd = -1;
        }
    }

private:
    int m_sockfd = -1;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};