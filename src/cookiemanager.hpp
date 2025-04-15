#pragma once
#include <map>
#include <string>
#include <optional>
#include "server.hpp"

class CookieManager {
public:
    static std::map<std::string, std::string> parseCookies(const std::string &header) {
        std::map<std::string, std::string> cookies;
        std::stringstream ss(header);
        std::string cookie;

        while (std::getline(ss, cookie, ';')) {
            size_t pos = cookie.find('=');
            if (pos != std::string::npos) {
                std::string key = cookie.substr(0, pos);
                std::string value = cookie.substr(pos + 1);
                // 去掉可能的空格
                key = trim(key);
                value = trim(value);
                cookies[key] = value;
            }
        }

        return cookies;
    }

    static void setCookie(HttpResponse &res, const std::string &key, const std::string &value) {
        std::string cookie = key + "=" + value;
        cookie += "; Max-Age=300";
        res.setHeader("Set-Cookie", cookie);
    }

private:
    static std::string trim(const std::string &str) {
        size_t first = str.find_first_not_of(' ');
        size_t last = str.find_last_not_of(' ');
        return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
    }
};

class SessionManager {
public:
    SessionManager() :
        m_sessionTimeout(std::chrono::minutes(5)) {
    } // 默认有效时间为 5 分钟

    std::string createSession() {
        std::string sessionId = generateSessionId();
        m_sessions[sessionId].creationTime = std::chrono::system_clock::now();
        return sessionId;
    }

    bool validateSession(const std::string &sid) {
        auto it = m_sessions.find(sid);
        if (it != m_sessions.end()) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.creationTime);
            if (duration > m_sessionTimeout) {
                m_sessions.erase(it);
                return false;
            }
            return true;
        }
        return false;
    }

    // 设置用户信息，存储在 session 中
    void setUser(const std::string &sid, const std::string &user) {
        m_sessions[sid].user = user;
    }

    // 根据 Session ID 获取用户信息
    std::optional<std::string> getUser(const std::string &sid) {
        auto it = m_sessions.find(sid);
        if (it != m_sessions.end()) {
            return it->second.user;
        }
        return std::nullopt; // 如果没有该 Session ID 或者用户为空
    }

    // 从 Config 中获取有效期配置，默认为 5 分钟
    void loadConfig() {
        // 这里暂时没有实现读取配置文件，默认有效期为 5 分钟
        // 后期你可以从 ConfigCenter 中读取，替换这部分逻辑
    }

private:
    struct Session {
        std::chrono::system_clock::time_point creationTime; // 会话创建时间
        std::string user; // 用户信息
    };

    std::map<std::string, Session> m_sessions;
    std::chrono::minutes m_sessionTimeout; // 会话有效期，默认5分钟

    // 生成一个随机的 Session ID
    std::string generateSessionId() {
        return std::to_string(std::rand()); // 简单实现，生产环境要改进
    }
};
