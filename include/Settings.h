#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"

namespace UnblockableSettings {
    inline std::vector<std::string> triggerEvents = {
        "PowerAttack_Start_End",
        "weaponSwing",
        "weaponLeftSwing",
        "h2hAttack"
    };

    struct ChanceSettings {
        bool enabled = true;
        bool visualsEnabled = true;
        bool effectShaderEnabled = false;
        float effectShaderDuration = 2.0f;
        bool soundEnabled = true;
        bool slowTimeEnabled = true;
        float slowTimeMultiplier = 0.5f;
        int slowTimeDuration = 100;
        float baseChance = 10.0f;
        float healthMult = 0.4f;
        float aggressionMult = 5.0f;
        float skillMult = 0.25f;
        float globalDifficulty = 250.0f;
        bool staggerEnabled = false;
        float staggerMagnitude = 0.5f;
        bool magnetismEnabled = false;
    };

    struct UnblockableRule {
        std::string ruleName;
        RE::FormID perkID = 0;
        ChanceSettings settings;
    };

    // Variáveis Globais de Configuração Geral
    inline ChanceSettings normalAttacks;
    inline ChanceSettings powerAttacks;

    // Perks de Exclusão (NPC não participará da lógica se possuir o Perk)
    inline RE::FormID normalDisablePerk = 0;
    inline RE::FormID powerDisablePerk = 0;

    // Listas de Regras Customizadas por Perk
    inline std::vector<UnblockableRule> normalRules;
    inline std::vector<UnblockableRule> powerRules;

    // SISTEMA DE FALLBACK DINÂMICO
    // Retorna a regra específica por Perk associada ao Ator; se nenhuma for encontrada, retorna a Global.
    inline ChanceSettings GetSettingsForActor(RE::Actor* a_actor, bool isPower) {
        if (!a_actor) return isPower ? powerAttacks : normalAttacks;
        const auto& rulesList = isPower ? powerRules : normalRules;
        for (const auto& rule : rulesList) {
            if (rule.perkID != 0) {
                auto perk = RE::TESForm::LookupByID<RE::BGSPerk>(rule.perkID);
                if (perk && a_actor->HasPerk(perk)) {
                    return rule.settings;
                }
            }
        }
        return isPower ? powerAttacks : normalAttacks;
    }

    // SISTEMA DE EXCLUSÃO DINÂMICO
    // Retorna verdadeiro se o ator possuir o perk configurado para ignorar a mecânica.
    inline bool IsActorExcluded(RE::Actor* a_actor, bool isPower) {
        if (!a_actor) return false;
        RE::FormID disablePerkID = isPower ? powerDisablePerk : normalDisablePerk;
        if (disablePerkID != 0) {
            auto perk = RE::TESForm::LookupByID<RE::BGSPerk>(disablePerkID);
            if (perk && a_actor->HasPerk(perk)) {
                return true;
            }
        }
        return false;
    }

    void LoadLanguage();
    const char* GetLoc(const std::string& key, const char* defaultVal);
    void SaveSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s, rapidjson::Document::AllocatorType& allocator);
    void LoadSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s);
    void SaveRule(const UnblockableRule& rule, bool isPower);
    void LoadRules();
    bool DrawDropdown(const char* label, const std::string& category, RE::FormID& current_form_id, float customWidth = -1.0f);
    void DrawRulesUI(const char* label, std::vector<UnblockableRule>& rules, bool isPower, bool& changed);
    void DrawChanceUI(const char* label, ChanceSettings& s, bool& changed);
    void UnBlockEventsMenu();
    void UnBlockMenu();
    void UnBlockPowerMenu();
    void UnBlockRegister();
    void UnBlockLoad();
    void UnBlockSave();
}