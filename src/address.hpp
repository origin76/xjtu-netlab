#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

class Address
{
public:
    using ptr = std::shared_ptr<Address>;

    virtual ~Address() = default;

    virtual int getFamily() const = 0;
    virtual std::string toString() const = 0;
    virtual const struct sockaddr *getAddress() const = 0;
    virtual socklen_t getLength() const = 0; // 添加 getLength 方法
    virtual void setAddressInfo(const std::string &ip, uint16_t port) = 0;

    static Address::ptr getLocalAddress(int sock);
    static Address::ptr getPeerAddress(int sock);

    static Address::ptr createIPv4Address(uint16_t port, const std::string &ip = "0.0.0.0");
};

class IPv4Address : public Address
{
public:
    IPv4Address(uint16_t port, const std::string &ip = "0.0.0.0")
    {
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
        {
            std::memset(&addr_.sin_addr, 0, sizeof(addr_.sin_addr));
        }
    }

    IPv4Address(const struct sockaddr_in &addr) : addr_(addr) {}

    ~IPv4Address() override = default;

    int getFamily() const override
    {
        return AF_INET;
    }

    std::string toString() const override
    {
        char buffer[64];
        std::memset(buffer, 0, sizeof(buffer));
        inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));
        return std::string(buffer) + ":" + std::to_string(ntohs(addr_.sin_port));
    }

    const struct sockaddr *getAddress() const override
    {
        return reinterpret_cast<const struct sockaddr *>(&addr_);
    }

    socklen_t getLength() const override
    { // 实现 getLength 方法
        return sizeof(addr_);
    }

    void setAddressInfo(const std::string &ip, uint16_t port) override
    {
        addr_.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
        {
            std::memset(&addr_.sin_addr, 0, sizeof(addr_.sin_addr));
        }
    }

    uint16_t getPort() const
    {
        return ntohs(addr_.sin_port);
    }

    std::string getIP() const
    {
        char buffer[INET_ADDRSTRLEN];
        std::memset(buffer, 0, sizeof(buffer));
        inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));
        return buffer;
    }

private:
    struct sockaddr_in addr_;
};

// 获取本地地址
Address::ptr Address::getLocalAddress(int sock)
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr), &len) == -1)
    {
        return nullptr;
    }

    if (addr.ss_family == AF_INET)
    {
        return std::make_shared<IPv4Address>(*reinterpret_cast<struct sockaddr_in *>(&addr));
    }
    return nullptr;
}

// 获取对端地址
Address::ptr Address::getPeerAddress(int sock)
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(sock, reinterpret_cast<struct sockaddr *>(&addr), &len) == -1)
    {
        return nullptr;
    }

    if (addr.ss_family == AF_INET)
    {
        return std::make_shared<IPv4Address>(*reinterpret_cast<struct sockaddr_in *>(&addr));
    }
    return nullptr;
}

// 创建 IPv4 地址
Address::ptr Address::createIPv4Address(uint16_t port, const std::string &ip)
{
    return std::make_shared<IPv4Address>(port, ip);
}