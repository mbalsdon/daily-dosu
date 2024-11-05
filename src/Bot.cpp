#include "Bot.h"
#include "DosuConfig.h"
#include "ScrapePlayers.h"
#include "OsuWrapper.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Detail
{
std::string toHourString(int hour)
{
    std::string ret = "";
    if (hour < 10)
    {
        ret += "0";
    }
    
    ret += std::to_string(hour) + ":00";
    return ret;
}

int convertTo12Hour(int hour)
{
    if (hour == 0)
    {
        return 12;
    }
    else if (hour > 0 && hour <= 12)
    {
        return hour;
    }
    else
    {
        return hour - 12;
    }
}

std::string toHexString(int hour)
{
    std::stringstream ss;
    ss << std::hex << hour;
    return ss.str();
}
}

/**
 * Bot constructor.
 */
Bot::Bot(const std::string& botToken) 
    : m_bot(botToken)
{}

/**
 * Start the discord bot.
 */
void Bot::start()
{
    std::cout << "Bot::start - starting Discord bot..." << std::endl;

    m_bot.on_log(dpp::utility::cout_logger());

    m_bot.on_slashcommand([this](const dpp::slashcommand_t& event)
    {
        std::string cmdName = event.command.get_command_name();
        std::cout << "Bot - received slash command /" << cmdName << "; routing..." << std::endl;

        if (cmdName == k_cmdPing)
        {
            cmdPing(event);
        }
    });

    m_bot.on_ready([this](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            m_bot.global_command_create(dpp::slashcommand(k_cmdPing, "Ping pong!", m_bot.me.id));
        }
    });

    m_bot.start(dpp::st_wait);
}

/**
 * Run the ping command.
 */
void Bot::cmdPing(const dpp::slashcommand_t& event)
{
    event.reply("Pong!");
}

/**
 * Read user data and display it thru a discord embed.
 */
void Bot::scrapePlayersCallback(int hour)
{
    if ((hour < 0) || (hour > 23))
    {
        hour = hour % 24;
        std::cout << "Bot::scrapePlayersCallback - hour is out of bounds; normalizing to " << hour << std::endl;
    }

    // Read data into memory
    nlohmann::json usersJson;
    try
    {
        std::filesystem::path rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
        std::filesystem::path dataFilePath = rootDir / k_dataDirName / k_jsonFileName;
        std::ifstream jsonFile(dataFilePath);
        if (!jsonFile.is_open())
        {
            std::string reason = std::string("Bot::scrapePlayersCallback - failed to open ").append(dataFilePath.string());
            throw std::runtime_error(reason);
        }

        usersJson = nlohmann::json::parse(jsonFile);
        if (!usersJson.is_array())
        {
            std::string reason = "Bot::scrapePlayersCallback - JSON content is not an array!";
            throw std::runtime_error(reason);
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    // Build discord embed
    std::string footer_text = "Runs every day at " + Detail::toHourString(hour) + "PST"; // FUTURE: clean (use chrono formatting, convert to UTC without knowing offset)
    std::string footer_icon = k_twemojiClockPrefix + Detail::toHexString(Detail::convertTo12Hour(hour) - 1) + k_twemojiClockSuffix;
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::dark_brown)
        .set_author("Here's your daily dose of osu!", "https://github.com/mbalsdon/daily-dosu", "") // zesty ahhh bot
        .set_timestamp(time(0))
        .set_footer(
            dpp::embed_footer()
            .set_text(footer_text)
            .set_icon(footer_icon)
        );

    std::stringstream desc;
    desc << std::fixed << std::setprecision(2);

    size_t displayUserMax = k_numDisplayUsers;
    size_t j = 1;
    for (size_t i = 0; i < displayUserMax; ++i)
    {
        nlohmann::json userJson = usersJson.at(i);

        if (userJson.at(k_userIDKey).is_null() ||
            userJson.at(k_usernameKey).is_null() ||
            userJson.at(k_countryCodeKey).is_null() ||
            userJson.at(k_pfpLinkKey).is_null() ||
            userJson.at(k_hoursPlayedKey).is_null() ||
            userJson.at(k_currentRankKey).is_null() ||
            userJson.at(k_yesterdayRankKey).is_null() ||
            userJson.at(k_rankChangeRatioKey).is_null())
        {
            ++displayUserMax;
            continue;
        }

        UserID userID = userJson.at(k_userIDKey).get<UserID>();
        Username username = userJson.at(k_usernameKey).get<Username>();
        CountryCode countryCode = userJson.at(k_countryCodeKey).get<CountryCode>();
        ProfilePicture pfpLink = userJson.at(k_pfpLinkKey).get<ProfilePicture>();
        HoursPlayed hoursPlayed = userJson.at(k_hoursPlayedKey).get<HoursPlayed>();
        Rank currentRank = userJson.at(k_currentRankKey).get<Rank>();
        Rank yesterdayRank = userJson.at(k_yesterdayRankKey).get<Rank>();
        PerformancePoints performancePoints = userJson.at(k_performancePointsKey).get<PerformancePoints>();
        Accuracy accuracy = userJson.at(k_accuracyKey).get<Accuracy>();
        RankChangeRatio rankChangeRatio = userJson.at(k_rankChangeRatioKey).get<RankChangeRatio>();

        std::transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        double rankChangePercent = rankChangeRatio * 100.0;

        desc << "**" << j << ".** :flag_" << countryCode << ": **[" << username << "](https://osu.ppy.sh/users/" << userID << "/osu)** **(" << performancePoints << "pp | " << accuracy << "% | " << hoursPlayed << "hrs)**\n";
        desc << "▸ Rank increased by " << rankChangePercent << "% (#" << yesterdayRank << " → #" << currentRank << ")\n";
        if (i != displayUserMax - 1)
        {
            desc << "▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬\n";
        }

        if (j == 1)
        {
            embed.set_thumbnail(pfpLink);
        }

        ++j;
    }

    embed.set_description(desc.str());
    m_bot.message_create(dpp::message(1001155170777960618, embed));
}
