#include "DosuConfig.h"
#include "Logger.h"
#include "Util.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>

int DosuConfig::logLevel;
std::string DosuConfig::discordBotToken;
std::string DosuConfig::osuClientID;
std::string DosuConfig::osuClientSecret;
int DosuConfig::osuApiCooldownMs;

/**
 * Load JSON configuration.
 */
void DosuConfig::load()
{
    LOG_DEBUG("Loading config from ", k_dosuConfigFilePath);

    nlohmann::json configDataJson;
    std::ifstream inputFile(k_dosuConfigFilePath);
    inputFile >> configDataJson;
    inputFile.close();

    DosuConfig::logLevel = configDataJson[k_logLevelKey];
    DosuConfig::discordBotToken = configDataJson[k_discordBotTokenKey];
    DosuConfig::osuClientID = configDataJson[k_osuClientIdKey];
    DosuConfig::osuClientSecret = configDataJson[k_osuClientSecretKey];
    DosuConfig::osuApiCooldownMs = configDataJson[k_osuApiCooldownMsKey];
}

/**
 * Check if config file exists.
 */
bool DosuConfig::configExists()
{
    return std::filesystem::exists(k_dosuConfigFilePath);
}

/**
 * Create config from user input.
 */
void DosuConfig::setupConfig()
{
    LOG_DEBUG("Running config setup tool...");

    nlohmann::json newConfigJson;
    newConfigJson[k_logLevelKey] = 1;
    newConfigJson[k_osuApiCooldownMsKey] = 1000;

    std::string botToken;
    std::string clientID;
    std::string clientSecret;

    std::cout << "In order to run this bot, you will need to register a Discord application and an osu!API client." << std::endl;
    std::cout << "Enter Discord bot token: ";
    std::getline(std::cin, botToken);
    std::cout << "Enter osu!API client ID: ";
    std::getline(std::cin, clientID);
    std::cout << "Enter osu!API client secret: ";
    std::getline(std::cin, clientSecret);

    newConfigJson[k_discordBotTokenKey] = botToken;
    newConfigJson[k_osuClientIdKey] = clientID;
    newConfigJson[k_osuClientSecretKey] = clientSecret;

    std::ofstream outputFile(k_dosuConfigFilePath);
    outputFile << newConfigJson.dump(4) << std::endl;

    std::cout << "Success! Some values have been set by default. Config can be found at " << k_dosuConfigFilePath << std::endl;
}
