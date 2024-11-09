#include "ServerConfig.h"
#include "Util.h"

#include <dpp/nlohmann/json.hpp>

#include <fstream>
#include <filesystem>

/**
 * ServerConfig constructor.
 */
ServerConfig::ServerConfig()
{
    load();
}

/**
 * Read server config from disk to memory.
 */
void ServerConfig::load()
{
    std::lock_guard<std::mutex> lock(m_serverConfigMtx);

    try
    {
        // Create default config if one doesn't exist
        if (!std::filesystem::exists(k_serverConfigFilePath))
        {
            std::ofstream newConfigFile(k_serverConfigFilePath);
            if (!newConfigFile.is_open())
            {
                std::string reason = std::string("ServerConfig::load - failed to open ").append(k_serverConfigFilePath.string());
                throw std::runtime_error(reason);
            }
            nlohmann::json jsonNewConfig = {{k_serverConfigChannelsKey, nlohmann::json::array()}};
            newConfigFile << jsonNewConfig.dump(2) << std::endl;
            newConfigFile.close();
        }

        // Read config into memory
        std::ifstream jsonFile(k_serverConfigFilePath);
        if (!jsonFile.is_open())
        {
            std::string reason = std::string("ServerConfig::load - failed to open ").append(k_serverConfigFilePath.string());
            throw std::runtime_error(reason);
        }

        nlohmann::json jsonServerConfig = nlohmann::json::parse(jsonFile);
        jsonFile.close();
        m_channelList.clear();
        for (const auto& channel : jsonServerConfig.at(k_serverConfigChannelsKey))
        {
            m_channelList.insert(channel.get<dpp::snowflake>());
        }
    }
    catch(const std::exception& e)
    {
        std::string reason = std::string("ServerConfig::load - ").append(e.what());
        throw std::runtime_error(reason);
    }
}

/**
 * Save server config from memory to disk.
 */
void ServerConfig::save()
{
    std::lock_guard<std::mutex> lock(m_serverConfigMtx);

    try
    {
        nlohmann::json jsonServerConfig;
        jsonServerConfig[k_serverConfigChannelsKey] = nlohmann::json::array();
        for (const auto& channel : m_channelList)
        {
            jsonServerConfig.at(k_serverConfigChannelsKey).push_back(channel);
        }

        std::filesystem::path tmpFilePath = k_serverConfigFilePath;
        tmpFilePath.replace_extension(".tmp");

        std::ofstream tmpFile(tmpFilePath);
        tmpFile << jsonServerConfig.dump(2);
        tmpFile.flush();
        tmpFile.close();

        std::filesystem::rename(tmpFilePath, k_serverConfigFilePath);
    }
    catch(const std::exception& e)
    {
        std::string reason = std::string("ServerConfig::save - ").append(e.what());
        throw std::runtime_error(reason);
    }
}

/**
 * Add a channel. Saves to disk immediately.
 */
void ServerConfig::addChannel(dpp::snowflake channelID)
{
    {
        std::lock_guard<std::mutex> lock(m_serverConfigMtx);
        if (m_channelList.find(channelID) != m_channelList.end())
        {
            return;
        }
        m_channelList.insert(channelID);
    }
    save();
}

/**
 * Remove a channel. Saves to disk immediately.
 */
void ServerConfig::removeChannel(dpp::snowflake channelID)
{
    {
        std::lock_guard<std::mutex> lock(m_serverConfigMtx);
        if (m_channelList.find(channelID) == m_channelList.end())
        {
            return;
        }
        m_channelList.erase(channelID);
    }
    save();
}

/**
 * Get list of channels.
 */
const std::unordered_set<dpp::snowflake> ServerConfig::getChannelList()
{
    std::lock_guard<std::mutex> lock(m_serverConfigMtx);
    return m_channelList;
}
