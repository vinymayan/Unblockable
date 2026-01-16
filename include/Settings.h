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
    };

    // Variáveis Globais
    inline ChanceSettings normalAttacks;
    inline ChanceSettings powerAttacks;

    void SaveSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s, rapidjson::Document::AllocatorType& allocator);

    void LoadSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s);

    void DrawChanceUI(const char* label, ChanceSettings& s, bool& changed);

    void UnBlockEventsMenu();

    // Funções do Menu
    void UnBlockMenu();
    void UnBlockPowerMenu();
    void UnBlockRegister();
    void UnBlockLoad();
    void UnBlockSave();
}