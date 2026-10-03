/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PlayerbotsPlusSchema.h"

#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"

namespace PlayerbotsPlus
{
// mod-playerbots creates playerbots_db_store.value as VARCHAR(255): an alt's non-combat list
// with every errands strategy is longer (MySQL 1406), and PlayerbotRepository::Save has already
// deleted the old row, so the bot loses its strategies at the next login. mod-playerbots' updater
// only reads its own SQL directories, hence a check here. Runs before the module config is read.
void EnsureDbStoreValueIsText()
{
    if (!PlayerbotsDatabase.Query("SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() "
                                  "AND TABLE_NAME = 'playerbots_db_store' AND COLUMN_NAME = 'value' "
                                  "AND DATA_TYPE = 'varchar' AND CHARACTER_MAXIMUM_LENGTH <= 255"))
        return;
    if (!sConfigMgr->GetOption<bool>("Playerbots.Updates.EnableDatabases", true))
    {
        LOG_WARN("server.loading", "playerbots-plus: playerbots_db_store.value is still VARCHAR(255): long "
                                   "strategy lists will not be saved (Playerbots database updates are disabled)");
        return;
    }
    PlayerbotsDatabase.DirectExecute("ALTER TABLE `playerbots_db_store` MODIFY COLUMN `value` TEXT NULL");
    LOG_INFO("server.loading", ">> playerbots-plus: playerbots_db_store.value widened to TEXT");
}
}  // namespace PlayerbotsPlus
