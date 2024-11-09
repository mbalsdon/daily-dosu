#ifndef __SERVER_CONFIG_H__
#define __SERVER_CONFIG_H__

#include <dpp/dpp.h>

#include <string>
#include <mutex>
#include <unordered_set>

const std::string k_serverConfigFileName = "server_config.json";

const std::string k_serverConfigChannelsKey = "channels";

class ServerConfig
{
public:
    ServerConfig();

    void load();
    void save();
    void addChannel(dpp::snowflake channelID);
    void removeChannel(dpp::snowflake channelID);
    const std::unordered_set<dpp::snowflake> getChannelList();

private:
    std::unordered_set<dpp::snowflake> m_channelList;
    mutable std::mutex m_serverConfigMtx; // mutable so we can lock in const methods
};

#endif /* __SERVER_CONFIG_H__ */
