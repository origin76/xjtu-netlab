#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <http_handler.hpp>

class ConfigParser {
public:
    explicit ConfigParser(const std::string &configFilePath) :
        m_configFile(configFilePath) {
        loadConfig();
    }

    void loadConfig() {
        try {
            boost::property_tree::ini_parser::read_ini(m_configFile, m_tree);
        } catch (const boost::property_tree::ini_parser_error &e) {
            spdlog::error("Error loading config file: {}", e.what());
        }
    }

    std::string getServerConfig(const std::string &key) {
        return m_tree.get<std::string>("server." + key);
    }

    std::string getSiteConfig(const std::string &key) {
        return m_tree.get<std::string>("site." + key);
    }

    std::string getUploadConfig(const std::string &key) {
        return m_tree.get<std::string>("upload." + key);
    }

    std::string getCookieConfig(const std::string &key) {
        return m_tree.get<std::string>("cookie." + key);
    }

    std::string getSessionConfig(const std::string &key) {
        return m_tree.get<std::string>("session." + key);
    }

    std::unordered_map<std::string, std::string> getSectionMap(const std::string &section) const {
        std::unordered_map<std::string, std::string> result;
        try {
            for (const auto &kv: m_tree.get_child(section)) {
                result[kv.first] = kv.second.get_value<std::string>();
            }
        } catch (const std::exception &e) {
            spdlog::warn("Section [{}] not found or invalid: {}", section, e.what());
        }
        return result;
    }

private:
    std::string m_configFile;
    boost::property_tree::ptree m_tree;
};

class ServerConfig {
public:
    explicit ServerConfig(ConfigParser &configParser) {
        try {
            int port = std::stoi(configParser.getServerConfig("port"));
            if (port < 0 || port > 65535)
                throw std::out_of_range("Invalid port");
            m_port = static_cast<uint16_t>(port);
        } catch (...) {
            spdlog::warn("Invalid or missing server.port, defaulting to 8080");
            m_port = 8080;
        }

        try {
            m_threads = std::stoi(configParser.getServerConfig("threads"));
        } catch (...) {
            spdlog::warn("Invalid or missing server.threads, defaulting to 4");
            m_threads = 4;
        }

        try {
            m_allowedIps = configParser.getServerConfig("allowed_ips");
        } catch (...) {
            spdlog::warn("Missing server.allowed_ips, defaulting to '0.0.0.0'");
            m_allowedIps = "0.0.0.0";
        }
    }

    uint16_t getPort() const { return m_port; }
    int getThreads() const { return m_threads; }
    std::string getAllowedIps() const { return m_allowedIps; }

private:
    uint16_t m_port;
    int m_threads;
    std::string m_allowedIps;
};

class SiteConfig {
public:
    explicit SiteConfig(ConfigParser &configParser) {
        try {
            m_rootDirectory = configParser.getSiteConfig("root_directory");
        } catch (...) {
            spdlog::warn("Missing site.root_directory, defaulting to './www'");
            m_rootDirectory = "./www";
        }

        try {
            m_defaultSite = configParser.getSiteConfig("default_site");
        } catch (...) {
            spdlog::warn("Missing site.default_site, defaulting to './index.html'");
            m_defaultSite = "./index.html";
        }
    }

    std::string getRootDirectory() const { return m_rootDirectory; }
    std::string getDefaultSite() const { return m_defaultSite; }

private:
    std::string m_rootDirectory;
    std::string m_defaultSite;
};

class ProxyConfig {
public:
    explicit ProxyConfig(ConfigParser &configParser) {
        auto proxyMap = configParser.getSectionMap("proxy");
        for (const auto &kv: proxyMap) {
            m_proxyMap[kv.first] = std::make_shared<ProxyHandler>(kv.second);
        }
    }

    const std::unordered_map<std::string, std::shared_ptr<ProxyHandler>> &getProxyMap() const {
        return m_proxyMap;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<ProxyHandler>> m_proxyMap;
};

class UploadConfig {
public:
    explicit UploadConfig(ConfigParser &configParser) {
        try {
            m_requestPath = configParser.getUploadConfig("request_path");
        } catch (...) {
            spdlog::warn("Missing upload.request_path, defaulting to '/upload'");
            m_requestPath = "/upload";
        }

        try {
            m_storagePath = configParser.getUploadConfig("storage_path");
        } catch (...) {
            spdlog::warn("Missing upload.storage_path, defaulting to './uploads'");
            m_storagePath = "./uploads";
        }
    }

    std::string getRequestPath() const { return m_requestPath; }
    std::string getStoragePath() const { return m_storagePath; }

private:
    std::string m_requestPath;
    std::string m_storagePath;
};

class CookieConfig {
public:
    explicit CookieConfig(ConfigParser &configParser) {
        try {
            m_expireTime = std::stoi(configParser.getCookieConfig("expire_time"));
        } catch (...) {
            spdlog::warn("Invalid or missing cookie.expire_time, defaulting to 300");
            m_expireTime = 300;
        }

        try {
            m_path = configParser.getCookieConfig("path");
        } catch (...) {
            spdlog::warn("Missing cookie.path, defaulting to '/'");
            m_path = "/";
        }
    }

    int getExpireTime() const { return m_expireTime; }
    std::string getPath() const { return m_path; }

private:
    int m_expireTime;
    std::string m_path;
};

class SessionConfig {
public:
    explicit SessionConfig(ConfigParser &configParser) {
        try {
            m_timeoutMinutes = std::stoi(configParser.getSessionConfig("timeout"));
        } catch (...) {
            spdlog::warn("Invalid or missing session.timeout, defaulting to 5 minutes");
            m_timeoutMinutes = 5;
        }
    }

    int getTimeoutMinutes() const { return m_timeoutMinutes; }

private:
    int m_timeoutMinutes;
};

class ConfigCenter {
public:
    static ConfigCenter &instance() {
        static ConfigCenter instance;
        return instance;
    }

    void init(const std::string &configFile) {
        m_configParser = std::make_shared<ConfigParser>(configFile);

        m_serverConfig = std::make_shared<ServerConfig>(*m_configParser);
        m_siteConfig = std::make_shared<SiteConfig>(*m_configParser);
        m_proxyConfig = std::make_shared<ProxyConfig>(*m_configParser);
        m_uploadConfig = std::make_shared<UploadConfig>(*m_configParser);
        m_cookieConfig = std::make_shared<CookieConfig>(*m_configParser);
        m_sessionConfig = std::make_shared<SessionConfig>(*m_configParser);
    }

    void printConfigInfo() {
        auto serverConfig = ConfigCenter::instance().getServerConfig();
        auto siteConfig = ConfigCenter::instance().getSiteConfig();
        auto uploadConfig = ConfigCenter::instance().getUploadConfig();
        auto cookieConfig = ConfigCenter::instance().getCookieConfig();
        auto sessionConfig = ConfigCenter::instance().getSessionConfig();
        auto proxyConfig = ConfigCenter::instance().getProxyConfig();

        spdlog::info("========= Loaded Configuration =========");
        spdlog::info("Server:");
        spdlog::info("  Port        : {}", serverConfig->getPort());
        spdlog::info("  Threads     : {}", serverConfig->getThreads());
        spdlog::info("  Allowed IPs : {}", serverConfig->getAllowedIps());

        spdlog::info("Site:");
        spdlog::info("  Root Dir    : {}", siteConfig->getRootDirectory());
        spdlog::info("  Default Site: {}", siteConfig->getDefaultSite());

        spdlog::info("Upload:");
        spdlog::info("  Request Path: {}", uploadConfig->getRequestPath());
        spdlog::info("  Storage Path: {}", uploadConfig->getStoragePath());

        spdlog::info("Cookie:");
        spdlog::info("  Path        : {}", cookieConfig->getPath());
        spdlog::info("  Expire Time : {} seconds", cookieConfig->getExpireTime());

        spdlog::info("Session:");
        spdlog::info("  Timeout     : {} minutes", sessionConfig->getTimeoutMinutes());

        spdlog::info("Proxy:");
        for (const auto &kv : proxyConfig->getProxyMap()) {
            spdlog::info("  PathPrefix: {} -> ProxyHandler", kv.first);
        }

        spdlog::info("=========================================");
    }

    std::shared_ptr<ServerConfig> getServerConfig() const { return m_serverConfig; }
    std::shared_ptr<SiteConfig> getSiteConfig() const { return m_siteConfig; }
    std::shared_ptr<ProxyConfig> getProxyConfig() const { return m_proxyConfig; }
    std::shared_ptr<UploadConfig> getUploadConfig() const { return m_uploadConfig; }
    std::shared_ptr<CookieConfig> getCookieConfig() const { return m_cookieConfig; }
    std::shared_ptr<SessionConfig> getSessionConfig() const { return m_sessionConfig; }


private:
    ConfigCenter() = default;
    ~ConfigCenter() = default;
    ConfigCenter(const ConfigCenter &) = delete;
    ConfigCenter &operator=(const ConfigCenter &) = delete;

    std::shared_ptr<ConfigParser> m_configParser;
    std::shared_ptr<ServerConfig> m_serverConfig;
    std::shared_ptr<SiteConfig> m_siteConfig;
    std::shared_ptr<ProxyConfig> m_proxyConfig;
    std::shared_ptr<UploadConfig> m_uploadConfig;
    std::shared_ptr<CookieConfig> m_cookieConfig;
    std::shared_ptr<SessionConfig> m_sessionConfig;
};
