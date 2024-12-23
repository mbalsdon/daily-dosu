# daily-dosu

A Discord bot for tracking [osu!](https://osu.ppy.sh) leaderboard changes. You can add the bot to your server with [this link](https://discord.com/oauth2/authorize?client_id=1109638103485907094).

## Features

Start your message with a slash (/) to run a command!

- **newsletter** - display the daily newsletter.
- **subscribe** - begin automatically sending daily newsletters to this channel.
- **unsubscribe** - stop sending daily newsletters to this channel.

![](media/newsletter_example.png)

## Running Locally

I have only tested on Linux. If you try running on other platforms, let me know how it goes!

1. To compile and run, you will need:
    - CMake 3.x
    - `libssl-dev`
    - `zlib1g-dev`
    - `libcurl4-openssl-dev`
2. After cloning the repository, initialize submodules:
    - `git submodule update --init --recursive`
3. Configure the project:
    - `mkdir <build folder path> && cd <build folder path>`
    - `cmake <daily-dosu path> -DCMAKE_BUILD_TYPE=Release`
4. Compile and run the project:
    - `` cmake --build . -j`nproc` ``
    - `./daily-dosu`


First-time users will be guided through a simple setup tool to generate a config file. You will need:
- A registered [osu! OAuth client](https://osu.ppy.sh/home/account/edit)
- A registered [Discord bot](https://discord.com/developers/applications)

## Contributing
If you find any bugs or want to request a feature, feel free to open an [issue](https://github.com/mbalsdon/daily-dosu/issues). If you want to make changes, feel free to open a PR. For direct contact, my DMs are open on Discord @spreadnuts.

## Internals

Core functionality can mainly be found in the following places:
- **ScrapeRankings** - script that fetches user data for the top 10,000 osu! players.
- **DailyJob** - implements a simple 24-hour job scheduler.
- **Bot** - DPP wrapper that handles all user-facing logic.
- **RankingsDatabaseManager** - SQLiteCpp wrapper for storing user data.
- **BotConfigDatabaseManager** - SQLiteCpp wrapper for storing Discord server configs (set by users).

Currently, `ScrapeRankings` is the only "big" job that is undertaken. It is registered along with a callback from `Bot` as a `DailyJob`, thus the flow of execution is roughly:
- Scheduler wakes up and runs `scrapeRankings`, which stores its results on disk.
- On completion, `scrapeRankingsCallback` runs, which loads the results from disk and formats them for Discord.
- Results are sent to any subscribed chat channels.
