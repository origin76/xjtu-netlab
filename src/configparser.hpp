#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <string>

class ConfigParser {
public:
    explicit ConfigParser(const std::string& configFilePath)
        : m_configFile(configFilePath) {
        loadConfig();
    }

    void loadConfig() {
        try {
            boost::property_tree::ini_parser::read_ini(m_configFile, m_tree);
        } catch (const boost::property_tree::ini_parser_error& e) {
            spdlog::error("Error loading config file: {}",e.what());
        }
    }

    std::string getServerConfig(const std::string& key) {
        return m_tree.get<std::string>("server." + key);
    }

    std::string getSiteConfig(const std::string& key) {
        return m_tree.get<std::string>("site." + key);
    }

private:
    std::string m_configFile;
    boost::property_tree::ptree m_tree;
};

class ServerConfig {
public:
    ServerConfig(const std::string& configFile) {
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
    SiteConfig(const std::string& configFile) {
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


class ConfigCenter {
public:
    static ConfigCenter& instance() {
        static ConfigCenter instance;
        return instance;
    }

    void init(const std::string& configFile) {
        m_serverConfig = std::make_shared<ServerConfig>(configFile);
        m_siteConfig = std::make_shared<SiteConfig>(configFile);
    }

    std::shared_ptr<ServerConfig> getServerConfig() const { return m_serverConfig; }
    std::shared_ptr<SiteConfig> getSiteConfig() const { return m_siteConfig; }

private:
    ConfigCenter() = default;
    ~ConfigCenter() = default;

    ConfigCenter(const ConfigCenter&) = delete;
    ConfigCenter& operator=(const ConfigCenter&) = delete;

    std::shared_ptr<ServerConfig> m_serverConfig;
    std::shared_ptr<SiteConfig> m_siteConfig;
};
