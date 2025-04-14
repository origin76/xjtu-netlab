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

    std::string getProxyConfig(const std::string &pathPrefix) {
        return m_tree.get<std::string>("proxy." + pathPrefix);
    }

    std::string getUploadConfig(const std::string &key) {
        return m_tree.get<std::string>("upload." + key);
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
    ServerConfig(const std::string &configFile) {
        ConfigParser configParser(configFile);
        int port = std::stoi(configParser.getServerConfig("port"));
        if (port < 0 or port > 65535) {
            throw std::out_of_range("Port must be between 0 and 65535");
        }
        m_port = static_cast<uint16_t>(port);
        m_threads = std::stoi(configParser.getServerConfig("threads"));
        m_allowedIps = configParser.getServerConfig("allowed_ips");
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
    SiteConfig(const std::string &configFile) {
        ConfigParser configParser(configFile);
        m_rootDirectory = configParser.getSiteConfig("root_directory");
        m_defaultSite = configParser.getSiteConfig("default_site");
    }

    std::string getRootDirectory() const { return m_rootDirectory; }
    std::string getDefaultSite() const { return m_defaultSite; }

private:
    std::string m_rootDirectory;
    std::string m_defaultSite;
};

class ProxyConfig {
public:
    explicit ProxyConfig(const std::string &configFile) {
        ConfigParser parser(configFile);
        auto proxyMap = parser.getSectionMap("proxy");
        for (const auto& kv : proxyMap) {
            // 为每个 pathPrefix 创建 ProxyHandler 实例并存储
            m_proxyMap[kv.first] = std::make_shared<ProxyHandler>(kv.second);
        }
    }

    const std::unordered_map<std::string, std::shared_ptr<ProxyHandler>>& getProxyMap() const {
        return m_proxyMap;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<ProxyHandler>> m_proxyMap;
};

class UploadConfig {
public:
    explicit UploadConfig(const std::string &configFile) {
        ConfigParser configParser(configFile);
        m_requestPath = configParser.getUploadConfig("request_path");
        m_storagePath = configParser.getUploadConfig("storage_path");
    }

    std::string getRequestPath() const { return m_requestPath; }
    std::string getStoragePath() const { return m_storagePath; }

private:
    std::string m_requestPath;
    std::string m_storagePath;
};


class ConfigCenter {
public:
    static ConfigCenter &instance() {
        static ConfigCenter instance;
        return instance;
    }

    void init(const std::string &configFile) {
        m_serverConfig = std::make_shared<ServerConfig>(configFile);
        m_siteConfig = std::make_shared<SiteConfig>(configFile);
        m_proxyConfig = std::make_shared<ProxyConfig>(configFile);
        m_uploadConfig = std::make_shared<UploadConfig>(configFile);
    }

    std::shared_ptr<ServerConfig> getServerConfig() const { return m_serverConfig; }
    std::shared_ptr<SiteConfig> getSiteConfig() const { return m_siteConfig; }
    std::shared_ptr<ProxyConfig> getProxyConfig() const { return m_proxyConfig; }
    std::shared_ptr<UploadConfig> getUploadConfig() const { return m_uploadConfig; }

private:
    ConfigCenter() = default;
    ~ConfigCenter() = default;
    ConfigCenter(const ConfigCenter &) = delete;
    ConfigCenter &operator=(const ConfigCenter &) = delete;

    std::shared_ptr<ServerConfig> m_serverConfig;
    std::shared_ptr<SiteConfig> m_siteConfig;
    std::shared_ptr<ProxyConfig> m_proxyConfig;
    std::shared_ptr<UploadConfig> m_uploadConfig;
};