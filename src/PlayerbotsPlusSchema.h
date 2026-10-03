/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SCHEMA_H
#define PLAYERBOTS_PLUS_SCHEMA_H

namespace PlayerbotsPlus
{
// Widens playerbots_db_store.value to TEXT while it is still VARCHAR(255) or less.
void EnsureDbStoreValueIsText();
}  // namespace PlayerbotsPlus

#endif
