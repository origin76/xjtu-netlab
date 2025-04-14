#pragma once
#include <map>
#include <string>
#include <optional>
#include "server.hpp"

class CookieManager {
public:
    static std::map<std::string, std::string> parseCookies(const std::string& header);
    static void setCookie(HttpResponse& res, const std::string& key, const std::string& value);
};

class SessionManager {
public:
    std::string createSession();
    bool validateSession(const std::string& sid);
    void setUser(const std::string& sid, const std::string& user);
    std::optional<std::string> getUser(const std::string& sid);
};
