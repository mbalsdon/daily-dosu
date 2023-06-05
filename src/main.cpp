#include "OsuWrapper.h"
#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"

#include <iostream>
#include <string>
#include <functional>


#include <thread>
#include <chrono>
void test()
{
    std::cout << "Hello world!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

/**
 * Entrypoint.
 * TODO: impl
 */
int main(int argc, char const *argv[])
{
    // TODO: Dynamic filepath
    DosuConfig::load("/home/mathew/dev/daily-dosu/dosu_config.json");

    // Start bot
    Bot bot(DosuConfig::discordBotToken);
    std::thread botThread(&Bot::start, &bot);

    // Give bot some time to start
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Start jobs
    DailyJob job(12, "myJob", test, [&bot]() { bot.todoRemoveMeCallback(); });
    std::thread jobThread(&DailyJob::start, &job);

    botThread.join();
    jobThread.join();

    return 0;

    // std::string clientID = DosuConfig::osuClientID;
    // std::string clientSecret = DosuConfig::osuClientSecret;
    // int apiCooldownMs = DosuConfig::osuApiCooldownMs;
    
    // OsuWrapper osu(clientID, clientSecret, apiCooldownMs);

    // DosuUser user;
    // UserID id = 6385683;
    // osu.getUser(id, user);
    // std::cout << user.username << std::endl;

    // UserID userIDs[50];
    // bool succ = osu.getRankingIDs(0, userIDs, 50);
    // for (int i = 0; i < 50; ++i) {
    //     std::cout << userIDs[i] << " ";
    // }
    // std::cout << std::endl;
}

// TODO: job impl, bot callback impl
// // store in json -> callback reads from json his ass is not using sqlite
// TODO: error behaviour when bot/job breaks??
// TODO: caching for job?
// TODO: per-server behaviour; start/stop -> do/dont execute callback
// TODO: go thru todos (readme, comments, etc)
// TODO: release
// TODO: callback filter options, sorting, etc?