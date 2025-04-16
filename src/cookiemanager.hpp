#pragma once
#include <map>
#include <string>
#include <optional>

#include "configparser.hpp"
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
                std::string key = trim(cookie.substr(0, pos));
                std::string value = trim(cookie.substr(pos + 1));
                cookies[key] = value;
            }
        }

        return cookies;
    }

    static void setCookie(HttpResponse &res, const std::string &key, const std::string &value) {
        auto config = ConfigCenter::instance().getCookieConfig();

        std::string cookie = key + "=" + value;
        cookie += "; Max-Age=" + std::to_string(config->getExpireTime());

        const std::string &path = config->getPath();
        if (!path.empty()) {
            cookie += "; Path=" + path;
        }

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
    SessionManager()
        : m_sessionTimeout(std::chrono::minutes(
              ConfigCenter::instance().getSessionConfig()->getTimeoutMinutes())) {}

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

    void setUser(const std::string &sid, const std::string &user) {
        m_sessions[sid].user = user;
    }

    std::optional<std::string> getUser(const std::string &sid) {
        auto it = m_sessions.find(sid);
        if (it != m_sessions.end()) {
            return it->second.user;
        }
        return std::nullopt;
    }

private:
    struct Session {
        std::chrono::system_clock::time_point creationTime;
        std::string user;
    };

    std::map<std::string, Session> m_sessions;
    std::chrono::minutes m_sessionTimeout;

    std::string generateSessionId() {
        return std::to_string(std::rand()); // 改进空间：使用 UUID
    }
};
