#ifndef __SCRAPE_RANKINGS_H__
#define __SCRAPE_RANKINGS_H__

#include "RankingsDatabase.h"
#include "TokenManager.h"

#include <memory>

void scrapeRankings(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<RankingsDatabase> pRankingsDb, bool bParallel);

#endif /* __SCRAPE_RANKINGS_H__ */
