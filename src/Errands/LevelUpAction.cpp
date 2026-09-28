/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "LevelUpAction.h"

#include "AiFactory.h"
#include "ChosenSpecValue.h"
#include "Event.h"
#include "LevelUpRules.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "Playerbots.h"

namespace PlayerbotsPlus
{
int32 PremadeSpecNo(Player* player, std::string const& name)
{
    uint8 const cls = player->getClass();
    for (int32 specNo = 0; specNo < MAX_SPECNO; ++specNo)
        if (!name.empty() && sPlayerbotAIConfig.premadeSpecName[cls][specNo] == name)
            return specNo;
    return -1;
}

bool LevelUpAction::Execute(Event /*event*/)
{
    // Gear kept for this level can be worn now (mod-playerbots does it for random bots only).
    botAI->DoSpecificAction("equip upgrade", Event("errands levelup"), true);

    std::string const& chosen = AI_VALUE(std::string&, "chosen spec");
    int32 const specNo = PremadeSpecNo(bot, chosen);
    std::map<uint8, uint32> tabs = AiFactory::GetPlayerSpecTabs(bot);
    uint32 const spent = tabs[0] + tabs[1] + tabs[2];

    switch (PickTalentSource(bot->GetFreeTalentPoints(), specNo >= 0, spent))
    {
        case TalentSource::None:
            return false;
        case TalentSource::Stored:
            PlayerbotFactory::InitTalentsBySpecNo(bot, specNo, false);
            break;
        case TalentSource::Current:
        {
            PlayerbotFactory factory(bot, bot->GetLevel());
            factory.InitTalentsTree(true, true, false);
            break;
        }
        case TalentSource::Ask:
            botAI->TellMaster("Choose my spec: whisper 'talents spec list', then 'talents spec <name>'");
            return false;
    }
    PlayerbotFactory(bot, bot->GetLevel()).InitPetTalents();
    botAI->TellMaster("Talent points spent");
    return true;
}
}  // namespace PlayerbotsPlus
