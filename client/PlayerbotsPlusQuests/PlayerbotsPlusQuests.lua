-- Playerbots Plus Quests: quest progress of the party's altbots (mod-playerbots-plus).
-- Bots answer "BOT\t#a questlog" with addon whispers, prefix PPQ:
--   Q<tab>id<tab>status<tab>title<tab>done/required:name|done/required:name...   then END
-- status: 0 in progress, 1 complete (to turn in), 2 failed.
-- Released under GNU GPL v2 or later.

local PREFIX = "PPQ"
local REQUEST = "#a questlog"
local ANSWER_TIMEOUT = 5    -- seconds before "(no answer)"
local REFRESH_EVERY = 10    -- seconds, while the window is shown
local THROTTLE = 2          -- seconds between event-driven refreshes
local LINE_HEIGHT = 14

local L = {
    title = "Quêtes du groupe",
    mine = "Mes quêtes",
    others = "Quêtes des alts",
    toTurnIn = "à rendre",
    failed = "échouée",
    noQuest = "n'a pas la quête",
    noAnswer = "(pas de réponse)",
    waiting = "(en attente...)",
    empty = "Aucune quête.",
}

local members = {}          -- name -> { quests = {[id] = quest}, building, askedAt, waiting, noAnswer }
local ownQuests = {}        -- [id] = quest, read from the client's own log
local lastRefresh, lastEventRefresh, elapsed = 0, 0, 0
local ui, content, linePool = nil, nil, {}

-- quest = { title = string, status = 0|1|2, objs = { { done, req, name } } }

-------------------------------------------------------------------------------
-- Data
-------------------------------------------------------------------------------

local function ReadOwnLog()
    local quests = {}
    for i = 1, GetNumQuestLogEntries() do
        local title, _, _, _, isHeader, _, isComplete = GetQuestLogTitle(i)
        local link = not isHeader and GetQuestLink(i)
        local id = link and tonumber(link:match("|Hquest:(%d+)"))
        if id then
            local quest = { title = title, status = (isComplete == 1 and 1) or (isComplete == -1 and 2) or 0, objs = {} }
            for j = 1, GetNumQuestLeaderBoards(i) do
                local text, _, finished = GetQuestLogLeaderBoard(j, i)
                local name, done, req = (text or ""):match("^(.-)%s*:%s*(%d+)%s*/%s*(%d+)")
                if name then
                    table.insert(quest.objs, { done = tonumber(done), req = tonumber(req), name = name })
                else
                    table.insert(quest.objs, { done = finished and 1 or 0, req = 1, name = text or "?" })
                end
            end
            quests[id] = quest
        end
    end
    return quests
end

local function ParseLine(msg)
    local tag, id, status, title, objectives = strsplit("\t", msg)
    if tag ~= "Q" or not tonumber(id) then
        return nil
    end
    local quest = { title = title or "?", status = tonumber(status) or 0, objs = {} }
    for part in (objectives or ""):gmatch("[^|]+") do
        local done, req, name = part:match("^(%d+)/(%d+):(.*)$")
        if done then
            table.insert(quest.objs, { done = tonumber(done), req = tonumber(req), name = name })
        end
    end
    return tonumber(id), quest
end

local function PartyNames()
    local names = {}
    for i = 1, GetNumPartyMembers() do
        local name = UnitName("party" .. i)
        if name then
            table.insert(names, name)
        end
    end
    table.sort(names)
    return names
end

local Render  -- forward

local function Refresh()
    lastRefresh = GetTime()
    ownQuests = ReadOwnLog()
    local inParty = {}
    for _, name in ipairs(PartyNames()) do
        inParty[name] = true
        local m = members[name] or { quests = {} }
        members[name] = m
        m.building, m.askedAt, m.waiting = {}, GetTime(), true
        SendAddonMessage("BOT", REQUEST, "WHISPER", name)
    end
    for name in pairs(members) do
        if not inParty[name] then
            members[name] = nil
        end
    end
    Render()
end

local function OnAddonMessage(prefix, msg, _, sender)
    if prefix ~= PREFIX then
        return
    end
    local m = members[sender]
    if not m then
        return
    end
    m.building = m.building or {}  -- also answers we did not ask for (a manual "questlog")
    if msg == "END" then
        m.quests, m.building, m.waiting, m.noAnswer = m.building, nil, false, false
        Render()
        return
    end
    local id, quest = ParseLine(msg)
    if id then
        m.building[id] = quest
    end
end

-------------------------------------------------------------------------------
-- Rendering
-------------------------------------------------------------------------------

local function Colour(obj, status)
    if status == 1 or obj.done >= obj.req then
        return "|cff40ff40"
    end
    if obj.done * 2 < obj.req then
        return "|cffff4040"
    end
    return "|cffffa030"
end

-- One line per member and objective: "Brogan  14/15 Red Leather Bandana".
local function MemberLines(lines, who, quest, dim)
    local prefix = "    " .. who .. "  "
    if not quest then
        table.insert(lines, { text = prefix .. "|cff808080" .. L.noQuest .. "|r" })
        return
    end
    if quest.status == 1 then
        table.insert(lines, { text = prefix .. "|cff40ff40" .. L.toTurnIn .. "|r" })
        return
    end
    if quest.status == 2 then
        table.insert(lines, { text = prefix .. "|cffff4040" .. L.failed .. "|r" })
        return
    end
    if #quest.objs == 0 then
        table.insert(lines, { text = prefix .. "|cffffa030...|r" })
    end
    for _, obj in ipairs(quest.objs) do
        local colour = dim and "|cff909090" or Colour(obj, quest.status)
        table.insert(lines, { text = prefix .. colour .. obj.done .. "/" .. obj.req .. "|r " .. obj.name })
    end
end

local function MemberState(name, m)
    if m.noAnswer then
        return "    " .. name .. "  |cff808080" .. L.noAnswer .. "|r"
    end
    if m.waiting and not next(m.quests) then
        return "    " .. name .. "  |cff808080" .. L.waiting .. "|r"
    end
end

local function SortedIds(set)
    local ids = {}
    for id, quest in pairs(set) do
        table.insert(ids, { id = id, title = quest.title })
    end
    table.sort(ids, function(a, b) return a.title < b.title end)
    return ids
end

local function BuildLines()
    local lines, names, me = {}, PartyNames(), UnitName("player")

    table.insert(lines, { text = "|cffffd100" .. L.mine .. "|r" })
    for _, entry in ipairs(SortedIds(ownQuests)) do
        table.insert(lines, { text = "  |cffffffff" .. entry.title .. "|r" })
        MemberLines(lines, me, ownQuests[entry.id], false)
        for _, name in ipairs(names) do
            local m = members[name]
            local state = m and MemberState(name, m)
            if state then
                table.insert(lines, { text = state })
            else
                MemberLines(lines, name, m and m.quests[entry.id], false)
            end
        end
    end
    if not next(ownQuests) then
        table.insert(lines, { text = "  |cff808080" .. L.empty .. "|r" })
    end

    -- Quests only alts have, greyed and collapsible.
    local others = {}
    for _, name in ipairs(names) do
        local m = members[name]
        for id, quest in pairs(m and m.quests or {}) do
            if not ownQuests[id] then
                others[id] = others[id] or { title = quest.title }
            end
        end
    end
    local otherIds = SortedIds(others)
    if #otherIds > 0 then
        local collapsed = PlayerbotsPlusQuestsDB.collapsed
        table.insert(lines, {
            text = "|cffffd100" .. (collapsed and "[+] " or "[-] ") .. L.others .. " (" .. #otherIds .. ")|r",
            toggle = true,
        })
        if not collapsed then
            for _, entry in ipairs(otherIds) do
                table.insert(lines, { text = "  |cff909090" .. entry.title .. "|r" })
                for _, name in ipairs(names) do
                    local quest = members[name] and members[name].quests[entry.id]
                    if quest then
                        MemberLines(lines, name, quest, true)
                    end
                end
            end
        end
    end
    return lines
end

local function GetLine(i)
    local line = linePool[i]
    if not line then
        line = CreateFrame("Button", nil, content)
        line:SetHeight(LINE_HEIGHT)
        line.text = line:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
        line.text:SetAllPoints()
        line.text:SetJustifyH("LEFT")
        line:SetScript("OnClick", function(self)
            if self.toggle then
                PlayerbotsPlusQuestsDB.collapsed = not PlayerbotsPlusQuestsDB.collapsed
                Render()
            end
        end)
        linePool[i] = line
    end
    return line
end

Render = function()
    if not ui or not ui:IsShown() then
        return
    end
    local lines = BuildLines()
    local width = ui.scroll:GetWidth()
    content:SetWidth(width)
    for i, data in ipairs(lines) do
        local line = GetLine(i)
        line:ClearAllPoints()
        line:SetPoint("TOPLEFT", content, "TOPLEFT", 0, -(i - 1) * LINE_HEIGHT)
        line:SetWidth(width)
        line.text:SetText(data.text)
        line.toggle = data.toggle
        line:Show()
    end
    for i = #lines + 1, #linePool do
        linePool[i]:Hide()
    end
    content:SetHeight(math.max(#lines * LINE_HEIGHT, 1))
end

-------------------------------------------------------------------------------
-- Window and minimap button
-------------------------------------------------------------------------------

local function SavePosition()
    local point, _, relPoint, x, y = ui:GetPoint()
    PlayerbotsPlusQuestsDB.pos = { point, relPoint, x, y }
    PlayerbotsPlusQuestsDB.size = { ui:GetWidth(), ui:GetHeight() }
end

local function CreateWindow()
    ui = CreateFrame("Frame", "PlayerbotsPlusQuestsFrame", UIParent)
    ui:SetBackdrop({
        bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
        edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
        tile = true, tileSize = 32, edgeSize = 24,
        insets = { left = 6, right = 6, top = 6, bottom = 6 },
    })
    local size = PlayerbotsPlusQuestsDB.size or { 360, 420 }
    ui:SetWidth(size[1])
    ui:SetHeight(size[2])
    local pos = PlayerbotsPlusQuestsDB.pos
    if pos then
        ui:SetPoint(pos[1], UIParent, pos[2], pos[3], pos[4])
    else
        ui:SetPoint("CENTER")
    end
    ui:SetMovable(true)
    ui:SetResizable(true)
    ui:SetMinResize(240, 160)
    ui:SetClampedToScreen(true)
    ui:EnableMouse(true)
    ui:RegisterForDrag("LeftButton")
    ui:SetScript("OnDragStart", ui.StartMoving)
    ui:SetScript("OnDragStop", function(self)
        self:StopMovingOrSizing()
        SavePosition()
    end)
    ui:SetFrameStrata("MEDIUM")
    table.insert(UISpecialFrames, "PlayerbotsPlusQuestsFrame")  -- Escape closes it

    local title = ui:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    title:SetPoint("TOP", 0, -14)
    title:SetText(L.title)

    local close = CreateFrame("Button", nil, ui, "UIPanelCloseButton")
    close:SetPoint("TOPRIGHT", -4, -4)

    ui.scroll = CreateFrame("ScrollFrame", "PlayerbotsPlusQuestsScroll", ui, "UIPanelScrollFrameTemplate")
    ui.scroll:SetPoint("TOPLEFT", 14, -34)
    ui.scroll:SetPoint("BOTTOMRIGHT", -32, 16)
    content = CreateFrame("Frame", nil, ui.scroll)
    content:SetWidth(1)
    content:SetHeight(1)
    ui.scroll:SetScrollChild(content)

    local grip = CreateFrame("Button", nil, ui)
    grip:SetWidth(16)
    grip:SetHeight(16)
    grip:SetPoint("BOTTOMRIGHT", -6, 6)
    grip:SetNormalTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Up")
    grip:SetHighlightTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Highlight")
    grip:SetScript("OnMouseDown", function() ui:StartSizing("BOTTOMRIGHT") end)
    grip:SetScript("OnMouseUp", function()
        ui:StopMovingOrSizing()
        SavePosition()
        Render()
    end)

    ui:SetScript("OnShow", Refresh)
    ui:SetScript("OnSizeChanged", function() Render() end)
    ui:SetScript("OnUpdate", function(_, delta)
        elapsed = elapsed + delta
        if elapsed < 0.5 then
            return
        end
        elapsed = 0
        local now, changed = GetTime(), false
        for _, m in pairs(members) do
            if m.waiting and now - m.askedAt > ANSWER_TIMEOUT then
                m.waiting, m.noAnswer, m.building = false, true, nil
                changed = true
            end
        end
        if now - lastRefresh >= REFRESH_EVERY then
            Refresh()
        elseif changed then
            Render()
        end
    end)
    ui:Hide()
end

local function Toggle()
    if ui:IsShown() then
        ui:Hide()
    else
        ui:Show()
    end
end

local function PlaceMinimapButton(button)
    local angle = math.rad(PlayerbotsPlusQuestsDB.minimapAngle or 200)
    button:SetPoint("CENTER", Minimap, "CENTER", math.cos(angle) * 80, math.sin(angle) * 80)
end

local function CreateMinimapButton()
    local button = CreateFrame("Button", "PlayerbotsPlusQuestsMinimapButton", Minimap)
    button:SetWidth(31)
    button:SetHeight(31)
    button:SetFrameStrata("MEDIUM")
    button:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")
    local icon = button:CreateTexture(nil, "BACKGROUND")
    icon:SetTexture("Interface\\Icons\\INV_Misc_Book_09")
    icon:SetWidth(20)
    icon:SetHeight(20)
    icon:SetPoint("CENTER", 0, 1)
    local border = button:CreateTexture(nil, "OVERLAY")
    border:SetTexture("Interface\\Minimap\\MiniMap-TrackingBorder")
    border:SetWidth(53)
    border:SetHeight(53)
    border:SetPoint("TOPLEFT")
    button:RegisterForClicks("LeftButtonUp")
    button:RegisterForDrag("LeftButton")
    button:SetScript("OnClick", Toggle)
    button:SetScript("OnDragStart", function(self)
        self:SetScript("OnUpdate", function(me)
            local mx, my = Minimap:GetCenter()
            local cx, cy = GetCursorPosition()
            local scale = Minimap:GetEffectiveScale()
            PlayerbotsPlusQuestsDB.minimapAngle = math.deg(math.atan2(cy / scale - my, cx / scale - mx))
            me:ClearAllPoints()
            PlaceMinimapButton(me)
        end)
    end)
    button:SetScript("OnDragStop", function(self) self:SetScript("OnUpdate", nil) end)
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_LEFT")
        GameTooltip:AddLine(L.title)
        GameTooltip:AddLine("/pq", 1, 1, 1)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    PlaceMinimapButton(button)
end

-------------------------------------------------------------------------------
-- Events
-------------------------------------------------------------------------------

local events = CreateFrame("Frame")
events:RegisterEvent("PLAYER_LOGIN")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("QUEST_LOG_UPDATE")
events:RegisterEvent("PARTY_MEMBERS_CHANGED")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "PLAYER_LOGIN" then
        PlayerbotsPlusQuestsDB = PlayerbotsPlusQuestsDB or {}
        CreateWindow()
        CreateMinimapButton()
    elseif event == "CHAT_MSG_ADDON" then
        OnAddonMessage(...)
    elseif ui and ui:IsShown() and GetTime() - lastEventRefresh >= THROTTLE then
        lastEventRefresh = GetTime()
        Refresh()
    end
end)

SLASH_PLAYERBOTSPLUSQUESTS1 = "/pq"
SlashCmdList.PLAYERBOTSPLUSQUESTS = function()
    if ui then
        Toggle()
    end
end
