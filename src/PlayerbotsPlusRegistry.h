/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_REGISTRY_H
#define PLAYERBOTS_PLUS_REGISTRY_H

namespace PlayerbotsPlus
{
// Adds our strategies, triggers, actions and values to the shared contexts of
// the 10 class AI contexts of mod-playerbots. Idempotent. Returns false if
// mod-playerbots has not built its contexts yet.
bool EnsureRegistered();
}  // namespace PlayerbotsPlus

#endif
