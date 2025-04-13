#ifndef __DAILY_TOP_PLAYS_H__
#define __DAILY_TOP_PLAYS_H__

#include "TopPlaysDatabase.h"
#include "TokenManager.h"

#include <memory>

void getTopPlays(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb, bool bParallel);

#endif /* __DAILY_TOP_PLAYS_H__ */
