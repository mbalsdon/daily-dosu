#ifndef __SCRAPE_RANKINGS_H__
#define __SCRAPE_RANKINGS_H__

#include "RankingsDatabase.h"
#include "TokenManager.h"
#include "ThreadPool.h"

#include <memory>

void scrapeRankings(
    std::shared_ptr<TokenManager> pTokenManager,
    std::shared_ptr<RankingsDatabase> pRankingsDb,
    std::shared_ptr<ThreadPool> pThreadPool);

#endif /* __SCRAPE_RANKINGS_H__ */
