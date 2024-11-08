#ifndef __SCRAPE_PLAYERS_H__
#define __SCRAPE_PLAYERS_H__

#include <cstddef>
#include <string>

constexpr size_t k_firstRangeMax = 100;
constexpr size_t k_secondRangeMax = 1000;
constexpr size_t k_thirdRangeMax = 10000;

constexpr size_t k_numDisplayUsers = 20;

const std::string k_usersFullFileName = "users_full.json";
const std::string k_usersCompactFileName = "users_compact.json";
const std::string k_dataDirName = "data";

const std::string k_firstRangeKey = "first_range";
const std::string k_secondRangeKey = "second_range";
const std::string k_thirdRangeKey = "third_range";

const std::string k_topUsersKey = "top";
const std::string k_bottomUsersKey = "bottom";

void scrapePlayers();

#endif /* __SCRAPE_PLAYERS_H__ */