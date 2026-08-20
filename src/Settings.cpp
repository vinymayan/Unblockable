#include "Settings.h"
#include "Manager.h"
#include <algorithm>

const char* Old_UnblockPath = "Data/SKSE/Plugins/UnblockableHits.json";
const char* Legacy_UnblockPath = "Data/SKSE/Plugins/Unblock/Settings.json";
const char* UnblockPath = "Data/Viny Mods/Unblockable Hits/Settings.json";
const char* LANG_PATH = "Data/Viny Mods/Unblockable Hits/Language.json";
const char* LEGACY_LANG_PATH = "Data/SKSE/Plugins/Unblock/Language.json";
static std::unordered_map<std::string, std::string> LangMap;

namespace {
    const std::string RULES_DIR = "Data/Viny Mods/Unblockable Hits/Rules/";
    const std::string LEGACY_RULES_DIR = "Data/SKSE/Plugins/Unblock/Rules/";

    std::string RuleDir(bool isPower)
    {
        return RULES_DIR + (isPower ? "Power/" : "Normal/");
    }

    std::string LegacyRuleDir(bool isPower)
    {
        return LEGACY_RULES_DIR + (isPower ? "Power/" : "Normal/");
    }

    void MigrateFileIfNeeded(const std::string& newPath, const std::string& legacyPath)
    {
        if (std::filesystem::exists(newPath) || !std::filesystem::exists(legacyPath)) return;

        std::filesystem::create_directories(std::filesystem::path(newPath).parent_path());
        std::error_code ec;
        std::filesystem::copy_file(legacyPath, newPath, std::filesystem::copy_options::skip_existing, ec);
        if (ec) {
            logger::warn("[Unblockable Hits] Failed to migrate '{}' to '{}': {}", legacyPath, newPath, ec.message());
        }
    }

    void MigrateRuleDirectory(bool isPower)
    {
        const auto newDir = RuleDir(isPower);
        const auto legacyDir = LegacyRuleDir(isPower);
        std::filesystem::create_directories(newDir);
        if (!std::filesystem::exists(legacyDir)) return;

        for (const auto& entry : std::filesystem::directory_iterator(legacyDir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

            const auto targetPath = std::filesystem::path(newDir) / entry.path().filename();
            if (std::filesystem::exists(targetPath)) continue;

            std::error_code ec;
            std::filesystem::copy_file(entry.path(), targetPath, std::filesystem::copy_options::skip_existing, ec);
            if (ec) {
                logger::warn("[Unblockable Hits] Failed to migrate rule '{}' to '{}': {}", entry.path().string(), targetPath.string(), ec.message());
            }
        }
    }

    std::string GetEditorID(RE::TESForm* form)
    {
        if (!form) return {};
        const char* editorID = form->GetFormEditorID();
        return editorID ? editorID : "";
    }

    void AddFormIdentity(rapidjson::Document& doc, rapidjson::Document::AllocatorType& alloc, const char* key, RE::FormID formID)
    {
        auto form = RE::TESForm::LookupByID(formID);
        const auto editorID = GetEditorID(form);
        if (!editorID.empty()) {
            std::string editorKey = std::string(key) + "EditorID";
            rapidjson::Value jsonKey;
            jsonKey.SetString(editorKey.c_str(), static_cast<rapidjson::SizeType>(editorKey.size()), alloc);
            rapidjson::Value jsonValue;
            jsonValue.SetString(editorID.c_str(), static_cast<rapidjson::SizeType>(editorID.size()), alloc);
            doc.AddMember(jsonKey, jsonValue, alloc);
        }

        std::string formStr = FormUtil::NormalizeFormID(form);
        rapidjson::Value fallback;
        fallback.SetString(formStr.c_str(), static_cast<rapidjson::SizeType>(formStr.size()), alloc);
        doc.AddMember(rapidjson::Value(key, alloc).Move(), fallback, alloc);
    }

    RE::FormID ReadFormIdentity(const rapidjson::Document& doc, const char* key)
    {
        std::string editorKey = std::string(key) + "EditorID";
        if (doc.HasMember(editorKey.c_str()) && doc[editorKey.c_str()].IsString()) {
            if (auto form = RE::TESForm::LookupByEditorID(doc[editorKey.c_str()].GetString())) {
                return form->GetFormID();
            }
        }
        if (doc.HasMember(key) && doc[key].IsString()) {
            return FormUtil::FormIDFromString(doc[key].GetString());
        }
        if (doc.HasMember(key) && doc[key].IsUint()) {
            return doc[key].GetUint();
        }
        return 0;
    }
}

void UnblockableSettings::LoadLanguage() {
    LangMap.clear();
    MigrateFileIfNeeded(LANG_PATH, LEGACY_LANG_PATH);

    std::ifstream file(LANG_PATH, std::ios::binary);
    if (!file.is_open()) {
        std::ifstream legacyFile(LEGACY_LANG_PATH, std::ios::binary);
        if (!legacyFile.is_open()) {
            SKSE::log::warn("Não foi possível carregar o arquivo Language.json. Usando textos padrões.");
            return;
        }
        file.swap(legacyFile);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    if (jsonStr.size() >= 3 && (unsigned char)jsonStr[0] == 0xEF && (unsigned char)jsonStr[1] == 0xBB && (unsigned char)jsonStr[2] == 0xBF) {
        jsonStr.erase(0, 3);
    }

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    if (doc.HasParseError()) return;

    if (doc.IsObject()) {
        for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
            if (itr->value.IsObject()) {
                std::string category = itr->name.GetString();
                for (auto jtr = itr->value.MemberBegin(); jtr != itr->value.MemberEnd(); ++jtr) {
                    if (jtr->value.IsString()) {
                        LangMap[category + "." + jtr->name.GetString()] = jtr->value.GetString();
                    }
                }
            }
            else if (itr->value.IsString()) {
                LangMap[itr->name.GetString()] = itr->value.GetString();
            }
        }
    }
}

const char* UnblockableSettings::GetLoc(const std::string& key, const char* defaultVal) {
    auto it = LangMap.find(key);
    if (it != LangMap.end()) return it->second.c_str();
    return defaultVal;
}

inline std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline int GetIndexFromID(int id, const int* idArray, int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        if (idArray[i] == id) return i;
    }
    return 0;
}

void UnblockableSettings::SaveSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s, rapidjson::Document::AllocatorType& allocator) {
    std::string p = prefix;
    doc.AddMember(rapidjson::Value((p + "Enabled").c_str(), allocator).Move(), s.enabled, allocator);
    doc.AddMember(rapidjson::Value((p + "Visuals").c_str(), allocator).Move(), s.visualsEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "EffectShaderEnabled").c_str(), allocator).Move(), s.effectShaderEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "EffectShaderDur").c_str(), allocator).Move(), s.effectShaderDuration, allocator);
    doc.AddMember(rapidjson::Value((p + "Sound").c_str(), allocator).Move(), s.soundEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "StaggerEnabled").c_str(), allocator).Move(), s.staggerEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "StaggerMag").c_str(), allocator).Move(), s.staggerMagnitude, allocator);
    doc.AddMember(rapidjson::Value((p + "BaseWeight").c_str(), allocator).Move(), s.baseChance, allocator);
    doc.AddMember(rapidjson::Value((p + "HealthMult").c_str(), allocator).Move(), s.healthMult, allocator);
    doc.AddMember(rapidjson::Value((p + "AggroMult").c_str(), allocator).Move(), s.aggressionMult, allocator);
    doc.AddMember(rapidjson::Value((p + "SkillMult").c_str(), allocator).Move(), s.skillMult, allocator);
    doc.AddMember(rapidjson::Value((p + "Difficulty").c_str(), allocator).Move(), s.globalDifficulty, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeEnabled").c_str(), allocator).Move(), s.slowTimeEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeMult").c_str(), allocator).Move(), s.slowTimeMultiplier, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeDur").c_str(), allocator).Move(), s.slowTimeDuration, allocator);
    doc.AddMember(rapidjson::Value((p + "Magnetism").c_str(), allocator).Move(), s.magnetismEnabled, allocator);
}

void UnblockableSettings::LoadSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s) {
    std::string p = prefix;
    if (doc.HasMember((p + "Enabled").c_str())) s.enabled = doc[(p + "Enabled").c_str()].GetBool();
    if (doc.HasMember((p + "Visuals").c_str())) s.visualsEnabled = doc[(p + "Visuals").c_str()].GetBool();
    if (doc.HasMember((p + "EffectShaderEnabled").c_str())) s.effectShaderEnabled = doc[(p + "EffectShaderEnabled").c_str()].GetBool();
    if (doc.HasMember((p + "EffectShaderDur").c_str())) s.effectShaderDuration = doc[(p + "EffectShaderDur").c_str()].GetFloat();
    if (doc.HasMember((p + "Sound").c_str())) s.soundEnabled = doc[(p + "Sound").c_str()].GetBool();
    if (doc.HasMember((p + "StaggerEnabled").c_str())) s.staggerEnabled = doc[(p + "StaggerEnabled").c_str()].GetBool();
    if (doc.HasMember((p + "StaggerMag").c_str())) s.staggerMagnitude = doc[(p + "StaggerMag").c_str()].GetFloat();
    if (doc.HasMember((p + "BaseWeight").c_str())) s.baseChance = doc[(p + "BaseWeight").c_str()].GetFloat();
    if (doc.HasMember((p + "HealthMult").c_str())) s.healthMult = doc[(p + "HealthMult").c_str()].GetFloat();
    if (doc.HasMember((p + "AggroMult").c_str())) s.aggressionMult = doc[(p + "AggroMult").c_str()].GetFloat();
    if (doc.HasMember((p + "SkillMult").c_str())) s.skillMult = doc[(p + "SkillMult").c_str()].GetFloat();
    if (doc.HasMember((p + "Difficulty").c_str())) s.globalDifficulty = doc[(p + "Difficulty").c_str()].GetFloat();
    if (doc.HasMember((p + "SlowTimeEnabled").c_str())) s.slowTimeEnabled = doc[(p + "SlowTimeEnabled").c_str()].GetBool();
    if (doc.HasMember((p + "SlowTimeMult").c_str())) s.slowTimeMultiplier = doc[(p + "SlowTimeMult").c_str()].GetFloat();
    if (doc.HasMember((p + "SlowTimeDur").c_str())) s.slowTimeDuration = doc[(p + "SlowTimeDur").c_str()].GetInt();
    if (doc.HasMember((p + "Magnetism").c_str())) s.magnetismEnabled = doc[(p + "Magnetism").c_str()].GetBool();
}

bool UnblockableSettings::DrawDropdown(const char* label, const std::string& category, RE::FormID& current_form_id, float customWidth) {
    bool changed = false;
    const auto& fullList = Manager::GetSingleton()->GetList(category);
    if (fullList.empty()) return false;

    std::vector<const char*> comboItems;
    std::vector<int> mapToFull;

    comboItems.push_back("None");
    mapToFull.push_back(-1);

    int localSelection = 0;
    for (size_t i = 0; i < fullList.size(); ++i) {
        comboItems.push_back(fullList[i].cachedDisplayName.c_str());
        mapToFull.push_back(static_cast<int>(i));
        if (fullList[i].formID == current_form_id) {
            localSelection = static_cast<int>(i) + 1;
        }
    }

    ImGuiMCP::PushID(label);
    std::string displayLabel = label;
    size_t hashPos = displayLabel.find("##");
    if (hashPos != std::string::npos) displayLabel = displayLabel.substr(0, hashPos);

    ImGuiMCP::Text("%s:", displayLabel.c_str());
    ImGuiMCP::SameLine();

    if (customWidth > 0.0f) ImGuiMCP::SetNextItemWidth(customWidth);
    const char* previewValue = comboItems[localSelection];

    if (ImGuiMCP::BeginCombo("##drop", previewValue)) {
        static std::map<std::string, std::string> searchBuffers;
        char searchBuf[256] = "";
        if (searchBuffers.contains(label)) strcpy_s(searchBuf, searchBuffers[label].c_str());

        ImGuiMCP::SetNextItemWidth(-1.0f);
        if (ImGuiMCP::InputText("##busca", searchBuf, sizeof(searchBuf))) {
            searchBuffers[label] = searchBuf;
        }
        ImGuiMCP::Separator();

        std::string searchStr = searchBuf;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), [](unsigned char c) { return std::tolower(c); });

        ImGuiMCP::BeginChild("##scroll", { 0, 200 }, false);
        for (int i = 0; i < comboItems.size(); i++) {
            std::string itemLower = comboItems[i];
            std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), [](unsigned char c) { return std::tolower(c); });

            if (searchStr.empty() || itemLower.find(searchStr) != std::string::npos) {
                bool isSelected = (localSelection == i);
                if (ImGuiMCP::Selectable(comboItems[i], isSelected)) {
                    localSelection = i;
                    int originalIndex = mapToFull[localSelection];

                    if (originalIndex == -1) current_form_id = 0;
                    else current_form_id = fullList[originalIndex].formID;

                    searchBuffers[label] = "";
                    changed = true;
                }
                if (isSelected) ImGuiMCP::SetItemDefaultFocus();
            }
        }
        ImGuiMCP::EndChild();
        ImGuiMCP::EndCombo();
    }
    ImGuiMCP::PopID();
    return changed;
}

void UnblockableSettings::SaveRule(const UnblockableRule& rule, bool isPower) {
    std::string dir = RuleDir(isPower);

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        logger::error("[Unblockable Hits] Error creating rules directory: {} -> {}", dir, ec.message());
        return;
    }

    std::string filepath = dir + rule.ruleName + ".json";
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    rapidjson::Value rName; rName.SetString(rule.ruleName.c_str(), alloc);
    doc.AddMember("ruleName", rName, alloc);

    AddFormIdentity(doc, alloc, "perk", rule.perkID);

    SaveSettingsInternal(doc, "", const_cast<ChanceSettings&>(rule.settings), alloc);

    FILE* fp = nullptr;
    fopen_s(&fp, filepath.c_str(), "wb");
    if (fp) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);
        fclose(fp);
    }
}

void UnblockableSettings::LoadRules() {
    normalRules.clear();
    powerRules.clear();

    MigrateRuleDirectory(false);
    MigrateRuleDirectory(true);

    std::string normalDir = RuleDir(false);
    std::string powerDir = RuleDir(true);

    auto loadFromDir = [](const std::string& dir, std::vector<UnblockableRule>& rulesList) {
        if (!std::filesystem::exists(dir)) return;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".json") {
                FILE* fp = nullptr;
                fopen_s(&fp, entry.path().string().c_str(), "rb");
                if (!fp) continue;

                char readBuffer[65536];
                rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
                rapidjson::Document doc;
                doc.ParseStream(is);
                fclose(fp);

                if (doc.HasParseError() || !doc.IsObject()) continue;

                UnblockableRule rule;
                if (doc.HasMember("ruleName")) rule.ruleName = doc["ruleName"].GetString();

                rule.perkID = ReadFormIdentity(doc, "perk");

                LoadSettingsInternal(doc, "", rule.settings);
                rulesList.push_back(rule);
            }
        }
        };

    loadFromDir(normalDir, normalRules);
    loadFromDir(powerDir, powerRules);
}

void UnblockableSettings::DrawRulesUI(const char* label, std::vector<UnblockableRule>& rules, bool isPower, bool& changed) {
    ImGuiMCP::Spacing();
    ImGuiMCP::Separator();
    ImGuiMCP::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "%s %s", label, GetLoc("menu.rules_header", "Rules (By Perk)"));

    if (ImGuiMCP::Button((std::string("+ ") + GetLoc("menu.add_rule", "Add Rule") + "##" + label).c_str())) {
        UnblockableRule newRule;
        newRule.ruleName = "New Rule " + std::to_string(rules.size() + 1);
        rules.push_back(newRule);
        changed = true;
    }

    ImGuiMCP::Spacing();

    for (size_t i = 0; i < rules.size(); ) {
        auto& rule = rules[i];
        ImGuiMCP::PushID(static_cast<int>(i));

        if (ImGuiMCP::CollapsingHeader(rule.ruleName.c_str())) {
            ImGuiMCP::Indent();

            char nameBuf[128];
            strcpy_s(nameBuf, rule.ruleName.c_str());
            if (ImGuiMCP::InputText(GetLoc("menu.rule_name", "Rule Name"), nameBuf, sizeof(nameBuf))) {
                std::string newName(nameBuf);
                if (newName != rule.ruleName && !newName.empty()) {
                    std::string oldDir = RuleDir(isPower);
                    std::string oldPath = oldDir + rule.ruleName + ".json";
                    if (std::filesystem::exists(oldPath)) std::filesystem::remove(oldPath);
                    rule.ruleName = newName;
                    changed = true;
                }
            }

            RE::FormID prevPerk = rule.perkID;
            std::string perkLabel = std::string(GetLoc("menu.target_perk", "Target Perk")) + "##" + std::to_string(i);
            if (DrawDropdown(perkLabel.c_str(), "Perk", rule.perkID, 300.0f)) {
                bool conflict = false;
                if (rule.perkID != 0) {
                    for (size_t j = 0; j < rules.size(); j++) {
                        if (i != j && rules[j].perkID == rule.perkID) {
                            conflict = true;
                            break;
                        }
                    }
                }
                if (conflict) {
                    rule.perkID = prevPerk;
                }
                else {
                    changed = true;
                }
            }

            ImGuiMCP::Separator();
            bool ruleChanged = false;
            DrawChanceUI((rule.ruleName + " " + GetLoc("menu.settings", "Settings")).c_str(), rule.settings, ruleChanged);
            if (ruleChanged) changed = true;
            ImGuiMCP::Separator();

            ImGuiMCP::Spacing();
            if (ImGuiMCP::Button(GetLoc("menu.remove_rule", "Remove Rule"), { 150, 0 })) {
                std::string dir = RuleDir(isPower);
                std::string path = dir + rule.ruleName + ".json";
                if (std::filesystem::exists(path)) std::filesystem::remove(path);
                rules.erase(rules.begin() + i);
                changed = true;
                ImGuiMCP::PopID();
                continue;
            }

            ImGuiMCP::Unindent();
        }
        ImGuiMCP::PopID();
        i++;
    }
}

void UnblockableSettings::DrawChanceUI(const char* label, ChanceSettings& s, bool& changed) {
    if (ImGuiMCP::CollapsingHeader(label, ImGuiMCP::ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.enabled", "Enabled")) + "##" + label).c_str(), &s.enabled)) changed = true;
        if (s.enabled) {
            ImGuiMCP::Indent();
            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.visual_effects", "Visual Effects")) + "##" + label).c_str(), &s.visualsEnabled)) changed = true;
            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.effect_shader", "Effect Shader")) + "##" + label).c_str(), &s.effectShaderEnabled)) changed = true;
            if (s.effectShaderEnabled) {
                ImGuiMCP::Indent();

                ImGuiMCP::SetNextItemWidth(250.0f);
                if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.shader_duration", "Shader Duration (s)")) + "##" + label).c_str(), &s.effectShaderDuration, 0.1f, 10.0f, "%.1f")) changed = true;
                ImGuiMCP::SameLine();
                ImGuiMCP::SetNextItemWidth(90.0f);
                if (ImGuiMCP::InputFloat((std::string("##ShaderDurPrecise") + label).c_str(), &s.effectShaderDuration, 0.0f, 0.0f, "%.1f")) {
                    s.effectShaderDuration = std::clamp(s.effectShaderDuration, 0.1f, 60.0f);
                    changed = true;
                }

                ImGuiMCP::Unindent();
            }
            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.sound_effects", "Sound Effects")) + "##" + label).c_str(), &s.soundEnabled)) changed = true;
            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.stagger_hit", "Stagger on Hit")) + "##" + label).c_str(), &s.staggerEnabled)) changed = true;
            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.magnetism", "Attack Magnetism")) + "##" + label).c_str(), &s.magnetismEnabled)) changed = true;
            if (ImGuiMCP::IsItemHovered()) {
                ImGuiMCP::SetTooltip(GetLoc("menu.magnetism_tooltip", "Only works if TCB is installed."));
            }
            ImGuiMCP::Indent();

            // --- Base Weight ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.base_weight", "Base Weight")) + "##" + label).c_str(), &s.baseChance, 0.0f, 100.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(90.0f);
            if (ImGuiMCP::InputFloat((std::string("##BasePrecise") + label).c_str(), &s.baseChance, 0.0f, 0.0f, "%.1f")) {
                s.baseChance = std::clamp(s.baseChance, 0.0f, 500.0f);
                changed = true;
            }

            // --- Health Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.health_mult", "Missing Health Mult")) + "##" + label).c_str(), &s.healthMult, 0.0f, 100.0f, "%.2f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(90.0f);
            if (ImGuiMCP::InputFloat((std::string("##HealthPrecise") + label).c_str(), &s.healthMult, 0.0f, 0.0f, "%.2f")) {
                s.healthMult = std::clamp(s.healthMult, 0.0f, 500.0f);
                changed = true;
            }

            // --- Aggression Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.aggro_mult", "Aggression Mult")) + "##" + label).c_str(), &s.aggressionMult, 0.0f, 50.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(90.0f);
            if (ImGuiMCP::InputFloat((std::string("##AggroPrecise") + label).c_str(), &s.aggressionMult, 0.0f, 0.0f, "%.1f")) {
                s.aggressionMult = std::clamp(s.aggressionMult, 0.0f, 500.0f);
                changed = true;
            }

            // --- Skill Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.skill_mult", "Skill Mult")) + "##" + label).c_str(), &s.skillMult, 0.0f, 5.0f, "%.2f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(90.0f);
            if (ImGuiMCP::InputFloat((std::string("##SkillPrecise") + label).c_str(), &s.skillMult, 0.0f, 0.0f, "%.2f")) {
                s.skillMult = std::clamp(s.skillMult, 0.0f, 100.0f);
                changed = true;
            }

            // --- Global Difficulty ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.difficulty", "Global Difficulty")) + "##" + label).c_str(), &s.globalDifficulty, 1.0f, 1000.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(90.0f);
            if (ImGuiMCP::InputFloat((std::string("##DiffPrecise") + label).c_str(), &s.globalDifficulty, 0.0f, 0.0f, "%.1f")) {
                s.globalDifficulty = std::clamp(s.globalDifficulty, 1.0f, 5000.0f);
                changed = true;
            }

            if (ImGuiMCP::Checkbox((std::string(GetLoc("menu.slow_time", "Slow Time on Trigger")) + "##" + label).c_str(), &s.slowTimeEnabled)) changed = true;
            if (s.slowTimeEnabled) {
                ImGuiMCP::Indent();
                ImGuiMCP::SetNextItemWidth(200.0f);
                if (ImGuiMCP::SliderFloat((std::string(GetLoc("menu.time_mult", "Time Multiplier")) + "##" + label).c_str(), &s.slowTimeMultiplier, 0.05f, 1.0f, "%.2f")) changed = true;
                ImGuiMCP::SetNextItemWidth(200.0f);
                if (ImGuiMCP::SliderInt((std::string(GetLoc("menu.duration_ms", "Duration (ms)")) + "##" + label).c_str(), &s.slowTimeDuration, 100, 5000)) changed = true;
                ImGuiMCP::Unindent();
            }
            ImGuiMCP::Separator();

            ImGuiMCP::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "%s", GetLoc("menu.probability_logic", "Probability Logic:"));
            ImGuiMCP::Spacing();

            // --- Bandit Simulations ---
            float banditPower100 = s.baseChance + (0.0f * s.healthMult) + (0.8f * s.aggressionMult) + (15.0f * s.skillMult);
            float banditChance100 = (banditPower100 / (banditPower100 + s.globalDifficulty)) * 100.0f;

            float banditPower50 = s.baseChance + (0.75f * s.healthMult) + (1.0f * s.aggressionMult) + (15.0f * s.skillMult);
            float banditChance50 = (banditPower50 / (banditPower50 + s.globalDifficulty)) * 100.0f;

            // --- New Boss / Skilled NPC Simulations ---
            float bossPower100 = s.baseChance + (0.0f * s.healthMult) + (2.0f * s.aggressionMult) + (40.0f * s.skillMult);
            float bossChance100 = (bossPower100 / (bossPower100 + s.globalDifficulty)) * 100.0f;

            float bossPower50 = s.baseChance + (0.5f * s.healthMult) + (3.0f * s.aggressionMult) + (100.0f * s.skillMult);
            float bossChance50 = (bossPower50 / (bossPower50 + s.globalDifficulty)) * 100.0f;

            // --- UI Render ---
            ImGuiMCP::BulletText(GetLoc("menu.sim_bandit_normal", "Normal Bandit - HP: 100%% -> Chance: %.2f%%"), banditChance100);
            ImGuiMCP::BulletText(GetLoc("menu.sim_bandit_injured", "Normal Bandit - HP: 25%% -> Chance: %.2f%%"), banditChance50);
            ImGuiMCP::BulletText(GetLoc("menu.sim_skilled_npc", "Skilled NPC - HP: 100%%, Aggro: 2.0, Weapon Skill: 40 -> Chance: %.2f%%"), bossChance100);
            ImGuiMCP::BulletText(GetLoc("menu.sim_high_skill_npc", "High Skill NPC - HP: 50%%, Aggro: 3.0, Weapon Skill: 100 -> Chance: %.2f%%"), bossChance50);

            ImGuiMCP::Unindent();
            ImGuiMCP::Unindent();
        }
    }
}

void UnblockableSettings::UnBlockEventsMenu() {
    static char newEventBuf[128] = "";
    bool changed = false;

    ImGuiMCP::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "%s", GetLoc("menu.animation_trigger_events", "Animation Trigger Events:"));
    ImGuiMCP::Separator();

    // 1. Obter referências de estilo e largura da janela
    ImGuiMCP::ImGuiStyle* style = ImGuiMCP::GetStyle();

    ImGuiMCP::ImVec2 contentRegionAvail;
    ImGuiMCP::GetContentRegionAvail(&contentRegionAvail);

    float availableWidth = contentRegionAvail.x;
    float currentX = 0.0f;
    float itemSpacing = style->ItemSpacing.x;
    float frameHeight = ImGuiMCP::GetFrameHeight();

    // Loop pelos eventos cadastrados
    for (size_t i = 0; i < triggerEvents.size(); ++i) {
        ImGuiMCP::PushID(static_cast<int>(i));

        // 2. Calcular a largura necessária para este item (Botão X + Espaçamento + Texto)
        ImGuiMCP::ImVec2 textSize;
        // O seu imguimcp exige 5 argumentos para CalcTextSize
        ImGuiMCP::CalcTextSize(&textSize, triggerEvents[i].c_str(), nullptr, false, 0.0f);

        // Largura total do "chip": Largura do botão + espaço interno + largura do texto
        float itemWidth = frameHeight + style->ItemInnerSpacing.x + textSize.x;

        // 3. Lógica de Quebra de Linha (Reflow)
        if (i > 0) {
            // Se o item atual + o espaçamento padrão ultrapassar a largura da janela
            if (currentX + itemSpacing + itemWidth > availableWidth) {
                // Não chama SameLine(), o que faz o cursor pular para a próxima linha
                currentX = 0.0f;
            }
            else {
                // Cabe na linha atual
                ImGuiMCP::SameLine(0.0f, itemSpacing);
                currentX += itemSpacing;
            }
        }

        // 4. Desenhar o grupo (Botão e Texto juntos)
        ImGuiMCP::BeginGroup();

        // Botão de deletar evento
        if (ImGuiMCP::Button("X", { frameHeight, frameHeight })) {
            triggerEvents.erase(triggerEvents.begin() + i);
            changed = true;
        }

        ImGuiMCP::SameLine(0.0f, style->ItemInnerSpacing.x);

        // Nome do evento
        ImGuiMCP::TextUnformatted(triggerEvents[i].c_str());

        ImGuiMCP::EndGroup();

        // Acumula a largura usada na linha atual
        currentX += itemWidth;

        ImGuiMCP::PopID();
    }

    ImGuiMCP::Spacing();
    ImGuiMCP::Separator();

    // Adição de novos eventos
    ImGuiMCP::InputText(GetLoc("menu.new_event_name", "New Event Name"), newEventBuf, sizeof(newEventBuf));
    if (ImGuiMCP::Button(GetLoc("menu.add_event", "Add Event")) && strlen(newEventBuf) > 0) {
        triggerEvents.push_back(newEventBuf);
        newEventBuf[0] = '\0';
        changed = true;
    }

    if (changed) {
        UnBlockSave();
    }
}

void UnblockableSettings::UnBlockMenu() {
    bool changed = false;
    if (DrawDropdown(GetLoc("menu.exclusion_normal", "Exclusion Perk (Disable Normal Attacks)"), "Perk", normalDisablePerk, 300.0f)) changed = true;
    ImGuiMCP::Separator();

    DrawChanceUI(GetLoc("menu.global_normal_title", "Normal Attacks Global Settings"), normalAttacks, changed);
    DrawRulesUI(GetLoc("menu.normal_label", "Normal Attack"), normalRules, false, changed);

    if (changed) UnBlockSave();
}

void UnblockableSettings::UnBlockPowerMenu() {
    bool changed = false;
    if (DrawDropdown(GetLoc("menu.exclusion_power", "Exclusion Perk (Disable Power Attacks)"), "Perk", powerDisablePerk, 300.0f)) changed = true;
    ImGuiMCP::Separator();

    DrawChanceUI(GetLoc("menu.global_power_title", "Power Attacks Global Settings"), powerAttacks, changed);
    DrawRulesUI(GetLoc("menu.power_label", "Power Attack"), powerRules, true, changed);

    if (changed) UnBlockSave();
}

void UnblockableSettings::UnBlockRegister() {
    if (SKSEMenuFramework::IsInstalled()) {
        LoadLanguage();
        SKSEMenuFramework::SetSection(GetLoc("menu.section", "Unblockable Hits"));
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.normal_attacks", "Normal Attacks"), UnBlockMenu);
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.power_attacks", "Power Attacks"), UnBlockPowerMenu);
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.animation_triggers", "Animation Triggers"), UnBlockEventsMenu);
    }
}

void UnblockableSettings::UnBlockLoad() {
    LoadLanguage(); // Inicializa o mapeador de tradução nativo
    MigrateFileIfNeeded(UnblockPath, Legacy_UnblockPath);
    MigrateFileIfNeeded(UnblockPath, Old_UnblockPath);

    FILE* fp = nullptr;
    fopen_s(&fp, UnblockPath, "rb");
    if (fp) {
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);
        if (doc.IsObject()) {
            LoadSettingsInternal(doc, "Normal", normalAttacks);
            LoadSettingsInternal(doc, "Power", powerAttacks);

            normalDisablePerk = ReadFormIdentity(doc, "NormalDisablePerk");
            powerDisablePerk = ReadFormIdentity(doc, "PowerDisablePerk");

            if (doc.HasMember("TriggerEvents") && doc["TriggerEvents"].IsArray()) {
                triggerEvents.clear();
                for (auto& v : doc["TriggerEvents"].GetArray()) {
                    if (v.IsString()) triggerEvents.push_back(v.GetString());
                }
            }
        }
    }
    LoadRules();
}

void UnblockableSettings::UnBlockSave() {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    SaveSettingsInternal(doc, "Normal", normalAttacks, allocator);
    SaveSettingsInternal(doc, "Power", powerAttacks, allocator);

    AddFormIdentity(doc, allocator, "NormalDisablePerk", normalDisablePerk);
    AddFormIdentity(doc, allocator, "PowerDisablePerk", powerDisablePerk);

    rapidjson::Value eventArray(rapidjson::kArrayType);
    for (const auto& evt : triggerEvents) {
        eventArray.PushBack(rapidjson::Value(evt.c_str(), allocator).Move(), allocator);
    }
    doc.AddMember("TriggerEvents", eventArray, allocator);

    std::filesystem::path path(UnblockPath);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        logger::error("[Unblockable Hits] Error creating base plugin folder path: {}", ec.message());
        return;
    }

    FILE* fp = nullptr;
    fopen_s(&fp, UnblockPath, "wb");
    if (fp) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);
        fclose(fp);
    }

    for (const auto& rule : normalRules) SaveRule(rule, false);
    for (const auto& rule : powerRules) SaveRule(rule, true);
}
