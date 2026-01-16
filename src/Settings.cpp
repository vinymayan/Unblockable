#include "Settings.h"

const char* UnblockPath = "Data/SKSE/Plugins/UnblockableHits.json";

void UnblockableSettings::SaveSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s, rapidjson::Document::AllocatorType& allocator) {
    std::string p = prefix;
    doc.AddMember(rapidjson::Value((p + "Enabled").c_str(), allocator).Move(), s.enabled, allocator);
    doc.AddMember(rapidjson::Value((p + "Visuals").c_str(), allocator).Move(), s.visualsEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "Sound").c_str(), allocator).Move(), s.soundEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "StaggerEnabled").c_str(), allocator).Move(), s.staggerEnabled, allocator); // Salvar Stagger
    doc.AddMember(rapidjson::Value((p + "StaggerMag").c_str(), allocator).Move(), s.staggerMagnitude, allocator);     // Salvar Magnitude
    doc.AddMember(rapidjson::Value((p + "BaseWeight").c_str(), allocator).Move(), s.baseChance, allocator);
    doc.AddMember(rapidjson::Value((p + "HealthMult").c_str(), allocator).Move(), s.healthMult, allocator);
    doc.AddMember(rapidjson::Value((p + "AggroMult").c_str(), allocator).Move(), s.aggressionMult, allocator);
    doc.AddMember(rapidjson::Value((p + "SkillMult").c_str(), allocator).Move(), s.skillMult, allocator);
    doc.AddMember(rapidjson::Value((p + "Difficulty").c_str(), allocator).Move(), s.globalDifficulty, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeEnabled").c_str(), allocator).Move(), s.slowTimeEnabled, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeMult").c_str(), allocator).Move(), s.slowTimeMultiplier, allocator);
    doc.AddMember(rapidjson::Value((p + "SlowTimeDur").c_str(), allocator).Move(), s.slowTimeDuration, allocator);
}

void UnblockableSettings::LoadSettingsInternal(rapidjson::Document& doc, const char* prefix, ChanceSettings& s) {
    std::string p = prefix;
    if (doc.HasMember((p + "Enabled").c_str())) s.enabled = doc[(p + "Enabled").c_str()].GetBool();
    if (doc.HasMember((p + "Visuals").c_str())) s.visualsEnabled = doc[(p + "Visuals").c_str()].GetBool();
    if (doc.HasMember((p + "Sound").c_str())) s.soundEnabled = doc[(p + "Sound").c_str()].GetBool();
    if (doc.HasMember((p + "StaggerEnabled").c_str())) s.staggerEnabled = doc[(p + "StaggerEnabled").c_str()].GetBool(); // Carregar Stagger
    if (doc.HasMember((p + "StaggerMag").c_str())) s.staggerMagnitude = doc[(p + "StaggerMag").c_str()].GetFloat();      // Carregar Magnitude
    if (doc.HasMember((p + "BaseWeight").c_str())) s.baseChance = doc[(p + "BaseWeight").c_str()].GetFloat();
    if (doc.HasMember((p + "HealthMult").c_str())) s.healthMult = doc[(p + "HealthMult").c_str()].GetFloat();
    if (doc.HasMember((p + "AggroMult").c_str())) s.aggressionMult = doc[(p + "AggroMult").c_str()].GetFloat();
    if (doc.HasMember((p + "SkillMult").c_str())) s.skillMult = doc[(p + "SkillMult").c_str()].GetFloat();
    if (doc.HasMember((p + "Difficulty").c_str())) s.globalDifficulty = doc[(p + "Difficulty").c_str()].GetFloat();
    if (doc.HasMember((p + "SlowTimeEnabled").c_str())) s.slowTimeEnabled = doc[(p + "SlowTimeEnabled").c_str()].GetBool();
    if (doc.HasMember((p + "SlowTimeMult").c_str())) s.slowTimeMultiplier = doc[(p + "SlowTimeMult").c_str()].GetFloat();
    if (doc.HasMember((p + "SlowTimeDur").c_str())) s.slowTimeDuration = doc[(p + "SlowTimeDur").c_str()].GetInt();
}

void UnblockableSettings::DrawChanceUI(const char* label, ChanceSettings& s, bool& changed) {
    if (ImGuiMCP::CollapsingHeader(label, ImGuiMCP::ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGuiMCP::Checkbox((std::string("Enabled##") + label).c_str(), &s.enabled)) changed = true;
        if (s.enabled) {
            ImGuiMCP::Indent();
            if (ImGuiMCP::Checkbox((std::string("Visual Effects##") + label).c_str(), &s.visualsEnabled)) changed = true;
            if (ImGuiMCP::Checkbox((std::string("Sound Effects##") + label).c_str(), &s.soundEnabled)) changed = true;
            if (ImGuiMCP::Checkbox((std::string("Stagger on Hit##") + label).c_str(), &s.staggerEnabled)) changed = true;
            ImGuiMCP::Indent();

            // --- Base Weight ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string("Base Weight##") + label).c_str(), &s.baseChance, 0.0f, 100.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(70.0f);
            if (ImGuiMCP::InputFloat((std::string("##BasePrecise") + label).c_str(), &s.baseChance, 0.0f, 0.0f, "%.1f")) {
                s.baseChance = std::clamp(s.baseChance, 0.0f, 500.0f);
                changed = true;
            }

            // --- Health Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string("Missing Health Mult##") + label).c_str(), &s.healthMult, 0.0f, 100.0f, "%.2f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(70.0f);
            if (ImGuiMCP::InputFloat((std::string("##HealthPrecise") + label).c_str(), &s.healthMult, 0.0f, 0.0f, "%.2f")) {
                s.healthMult = std::clamp(s.healthMult, 0.0f, 500.0f);
                changed = true;
            }

            // --- Aggression Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string("Aggression Mult##") + label).c_str(), &s.aggressionMult, 0.0f, 50.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(70.0f);
            if (ImGuiMCP::InputFloat((std::string("##AggroPrecise") + label).c_str(), &s.aggressionMult, 0.0f, 0.0f, "%.1f")) {
                s.aggressionMult = std::clamp(s.aggressionMult, 0.0f, 500.0f);
                changed = true;
            }

            // --- Skill Mult ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string("Skill Mult##") + label).c_str(), &s.skillMult, 0.0f, 5.0f, "%.2f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(70.0f);
            if (ImGuiMCP::InputFloat((std::string("##SkillPrecise") + label).c_str(), &s.skillMult, 0.0f, 0.0f, "%.2f")) {
                s.skillMult = std::clamp(s.skillMult, 0.0f, 100.0f);
                changed = true;
            }

            // --- Global Difficulty ---
            ImGuiMCP::SetNextItemWidth(250.0f);
            if (ImGuiMCP::SliderFloat((std::string("Global Difficulty##") + label).c_str(), &s.globalDifficulty, 1.0f, 1000.0f, "%.1f")) changed = true;
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(70.0f);
            if (ImGuiMCP::InputFloat((std::string("##DiffPrecise") + label).c_str(), &s.globalDifficulty, 0.0f, 0.0f, "%.1f")) {
                s.globalDifficulty = std::clamp(s.globalDifficulty, 1.0f, 5000.0f);
                changed = true;
            }

            if (ImGuiMCP::Checkbox((std::string("Slow Time on Trigger##") + label).c_str(), &s.slowTimeEnabled)) changed = true;
            if (s.slowTimeEnabled) {
                ImGuiMCP::Indent();

                ImGuiMCP::SetNextItemWidth(200.0f);
                if (ImGuiMCP::SliderFloat((std::string("Time Multiplier##") + label).c_str(), &s.slowTimeMultiplier, 0.05f, 1.0f, "%.2f")) changed = true;

                ImGuiMCP::SetNextItemWidth(200.0f);
                if (ImGuiMCP::SliderInt((std::string("Duration (ms)##") + label).c_str(), &s.slowTimeDuration, 100, 5000)) changed = true; // Slider de 100ms a 5000ms
                ImGuiMCP::Unindent();
            }
            ImGuiMCP::Separator();

            // --- Seção de Simulação e Fórmula ---
            ImGuiMCP::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "Probability Logic:");

            ImGuiMCP::Spacing();

            float banditPower100 = s.baseChance + (0.0f * s.healthMult) + (0.8f * s.aggressionMult) + (15.0f * s.skillMult);
            float banditChance100 = (banditPower100 / (banditPower100 + s.globalDifficulty)) * 100.0f;

            float banditPower50 = s.baseChance + (0.75f * s.healthMult) + (1.0f * s.aggressionMult) + (15.0f * s.skillMult);
            float banditChance50 = (banditPower50 / (banditPower50 + s.globalDifficulty)) * 100.0f;

            float bossPower100 = s.baseChance + (0.0f * s.healthMult) + (2.0f * s.aggressionMult) + (40.0f * s.skillMult);
            float bossChance100 = (bossPower100 / (bossPower100 + s.globalDifficulty)) * 100.0f;

            float bossPower50 = s.baseChance + (0.5f * s.healthMult) + (3.0f * s.aggressionMult) + (100.0f * s.skillMult);
            float bossChance50 = (bossPower50 / (bossPower50 + s.globalDifficulty)) * 100.0f;

            ImGuiMCP::BulletText("Normal Bandit  - HP: 100%%, Aggro: 0.8, Weapon Skill: 15 -> Chance: %.2f%%", banditChance100);
            ImGuiMCP::BulletText("Normal Bandit - HP: 25%%, Aggro: 1.0, Weapon Skill: 15 -> Chance: %.2f%%", banditChance50);
            ImGuiMCP::BulletText("Skilled NPC: 100%%, Aggro: 2.0, Weapon Skill: 40 -> Chance: %.2f%%", bossChance100);
            ImGuiMCP::BulletText("High Skill NPC: 50%%, Aggro: 3.0, Weapon Skill: 100 -> Chance: %.2f%%", bossChance50);

            ImGuiMCP::Unindent();

            ImGuiMCP::Unindent();
        }
    }
}

void UnblockableSettings::UnBlockEventsMenu() {
    static char newEventBuf[128] = "";
    bool changed = false;

    ImGuiMCP::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "Animation Trigger Events:");
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
    ImGuiMCP::InputText("New Event Name", newEventBuf, sizeof(newEventBuf));
    if (ImGuiMCP::Button("Add Event") && strlen(newEventBuf) > 0) {
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
    DrawChanceUI("Normal Attacks", normalAttacks, changed);

    if (changed) UnBlockSave();
}

void UnblockableSettings::UnBlockPowerMenu() {
    bool changed = false;
    DrawChanceUI("Power Attacks", powerAttacks, changed);
    if (changed) UnBlockSave();
}

void UnblockableSettings::UnBlockRegister() {
    if (SKSEMenuFramework::IsInstalled()) {
        SKSEMenuFramework::SetSection("Unblockable Hits");
        SKSEMenuFramework::AddSectionItem("Normal Attacks", UnBlockMenu);
        SKSEMenuFramework::AddSectionItem("Power Attacks", UnBlockPowerMenu);
        SKSEMenuFramework::AddSectionItem("Animation Triggers", UnBlockEventsMenu);
    }
}

void UnblockableSettings::UnBlockLoad() {
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
            if (doc.HasMember("TriggerEvents") && doc["TriggerEvents"].IsArray()) {
                triggerEvents.clear();
                for (auto& v : doc["TriggerEvents"].GetArray()) {
                    if (v.IsString()) triggerEvents.push_back(v.GetString());
                }
            }
        }
    }
}

void UnblockableSettings::UnBlockSave() {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    SaveSettingsInternal(doc, "Normal", normalAttacks, allocator);
    SaveSettingsInternal(doc, "Power", powerAttacks, allocator);
    rapidjson::Value eventArray(rapidjson::kArrayType);
    for (const auto& evt : triggerEvents) {
        eventArray.PushBack(rapidjson::Value(evt.c_str(), allocator).Move(), allocator);
    }
    doc.AddMember("TriggerEvents", eventArray, allocator);
    std::filesystem::path path(UnblockPath);
    std::filesystem::create_directories(path.parent_path());

    FILE* fp = nullptr;
    fopen_s(&fp, UnblockPath, "wb");
    if (fp) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);
        fclose(fp);
    }
}
