# Assigned Professions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `professions` command assigns 1–2 primary professions per alt; errands then trains them at trainers and buys their tools within the leash.

**Architecture:** Pure `ProfessionCatalog` (names, ids, tools, parsing, persistence format, forget choice) and two new pure errand kinds in `ErrandPlanner`. Core side: `Professions.{h,cpp}` (trainer profession, can-train, teach, tool purchase), a persisted `ProfessionsValue` and a `ProfessionsAction` chat command.

**Tech Stack:** as v0.1.0.

**Spec:** `docs/specs/2026-09-27-professions-design.md`

## Global Constraints

- Constraints of `docs/plans/2026-09-26-errands.md` apply; branch `professions`; remote `github`.
- Skill ids: Alchemy 171, Blacksmithing 164, Enchanting 333, Engineering 202, Herbalism 182, Inscription 773, Jewelcrafting 755, Leatherworking 165, Mining 186, Skinning 393, Tailoring 197.
- Tools: Mining 2901, Skinning 7005, Blacksmithing 5956, Engineering 6219, Jewelcrafting 20815, Inscription 39505.
- Forgetting a profession happens only at a trainer of an assigned profession that needs the slot, or on `professions reset confirm`.
- Persist the assignment with `PlayerbotRepository::instance().Save(botAI)` after every change.

---

### Task 1: ProfessionCatalog (pure, TDD)

**Files:** create `src/Errands/ProfessionCatalog.{h,cpp}`, `tests/ProfessionCatalogTest.cpp`; add to `tests/CMakeLists.txt`.

**Produces:**

```cpp
struct ProfessionInfo { char const* name; uint32_t skill; uint32_t tool; };  // tool 0: none to buy
std::vector<ProfessionInfo> const& PrimaryProfessions();
ProfessionInfo const* FindProfession(uint32_t skill);
struct ParsedAssignment { std::vector<uint32_t> skills; std::string error; bool Ok() const; };
ParsedAssignment ParseAssignment(std::string const& text);
std::string SaveAssignment(std::vector<uint32_t> const& skills);   // "186,197"
std::vector<uint32_t> LoadAssignment(std::string const& text);    // drops unknown ids
uint32_t ProfessionToForget(std::vector<std::pair<uint32_t, uint32_t>> const& known,  // (skill, value)
                            std::vector<uint32_t> const& assigned, uint32_t freeSlots, uint32_t wanted);
```

**Tests** (write first, see them fail, then implement):
- `Parse.NamesCaseInsensitive`: "Mining TAILORING" → {186, 197}.
- `Parse.OneProfessionIsFine`: "herbalism" → {182}.
- `Parse.RejectsThreeUnknownAndDuplicates`: errors `at most two primary professions`, `unknown profession: cooking`, `duplicate profession: mining`.
- `Persist.RoundTrip` and `Persist.LoadDropsUnknown` ("186,999,197" → {186, 197}; "" → {}).
- `Forget.NotNeededWithFreeSlotOrAlreadyKnown`, `Forget.LowestUnassigned`, `Forget.NeverAssignedOrWithoutAssignment`.
- `Catalog.ToolsMatchTheSpec`: mining 2901, skinning 7005, blacksmithing 5956, engineering 6219, jewelcrafting 20815, inscription 39505, others 0.

- [ ] Commit, push: `Profession catalog: names, tools, assignment parsing, forget choice`.

---

### Task 2: Planner kinds Train and BuyTool (pure, TDD)

**Files:** `src/Errands/ErrandPlanner.{h,cpp}`, `tests/ErrandPlannerTest.cpp`.

- `ErrandKind`: append `Train`, `BuyTool` (after `Sell`); `ToString`: `train`, `buy tool`.
- `Candidate`: `bool canTrain = false; bool canSellTool = false;`.
- `BestKind`: after the `Sell` rule, `if (vendorRested && c.canTrain) return Train; if (vendorRested && c.canSellTool) return BuyTool;`.

**Tests:** `Pick.TrainAndBuyToolComeAfterSell` (sell beats train beats buy tool), `Pick.TrainerRestsAfterVisit`.

- [ ] Commit, push: `Planner: Train and BuyTool errands`.

---

### Task 3: Core side, command, build

**Files:** create `src/Errands/Professions.{h,cpp}`, `src/Errands/ProfessionsValue.h`, `src/Errands/ProfessionsAction.{h,cpp}`; modify `RunErrandAction.cpp` (scan filter, `Describe`, `VisitTarget`), `ErrandsStrategy.h` (chat trigger `professions`), `PlayerbotsPlusRegistry.cpp`.

- `Professions.h`:

```cpp
uint32 TrainerSkill(Creature* npc);  // 0 if not a tradeskill trainer
bool CanTrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void TrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
bool MissingToolAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void BuyToolsAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
std::vector<std::pair<uint32, uint32>> KnownPrimaries(Player* bot);  // (skill, value)
```

- `TrainerSkill`: `sObjectMgr->GetTrainer(npc->GetEntry())`, type `Tradeskill`; first spell `ReqSkillLine`; else the `SPELL_EFFECT_SKILL_STEP` misc value of the spell (or of the spell it teaches via `SPELL_EFFECT_LEARN_SPELL`).
- `CanTrainAt`: skill assigned, trainer valid for the bot; if the bot lacks the skill and has no free primary point → true iff `ProfessionToForget(...)` finds one; else any trainer spell with `CanTeachSpell` and `HasEnoughMoney(cost × reputation discount)`.
- `TrainAt`: forget `ProfessionToForget(...)` with `SetSkill(skill, 0, 0, 0)` when needed; then up to 4 passes teaching every spell with `CanTeachSpell` and enough money via `Trainer::TeachSpell`; stop when a pass teaches nothing.
- `MissingToolAt` / `BuyToolsAt`: for each assigned profession with a tool the bot lacks (`HasItemCount(tool, 1, false)`), find it in `npc->GetVendorItems()`; price `BuyPrice × discount`; buy with `BuyItemFromVendorSlot(guid, slot, tool, 1, NULL_BAG, NULL_SLOT)`.
- `ProfessionsValue`: `ManualSetValue<ProfessionsData&>` named `"assigned professions"`; `ProfessionsData { std::vector<uint32> skills; uint32 resetAskedAt = 0; bool resetPending = false; }`; `Save()` → `SaveAssignment`, `Load()` → `LoadAssignment`.
- `ProfessionsAction` (`"professions"`): param `""` status; `clear`; `reset` (pending, 30 s); `reset confirm`; else `ParseAssignment` → store + repository save, or reply with the error.
- `RunErrandAction`: `Describe` sets `canTrain` (creature is a trainer) and `canSellTool` (creature is a vendor) from the helpers; scan keeps candidates with those flags; `VisitTarget` calls `TrainAt` / `BuyToolsAt` when relevant.
- Registry: value, action, chat trigger `ChatCommandTrigger("professions")`; `ErrandsStrategy` adds `TriggerNode("professions", {NextAction("professions", ChatCommandRelevance)})`.

- [ ] Unit tests, full build, commit, push: `Professions: assignment command, training and tools in errands`.

---

### Task 4: Docs, deploy, validate, release

- [ ] README section "Assigned professions" + commands rows; `docs/testing.md` section P1–P8 (spec's in-game list); `docs/upstream-dependencies.md` rows (`PlayerbotRepository::Save`, value `Save`/`Load` persistence); CHANGELOG.
- [ ] Commit, push; ask to restart; validate with the user; release v0.5.0.
