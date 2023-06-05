#include "Bot.h"
#include "DosuConfig.h"

#include <iostream>
#include <string>

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

// TODO: remove
void Bot::todoRemoveMeCallback()
{
    m_bot.message_create(dpp::message(1001155170777960618, "yooooooooo"));
}
