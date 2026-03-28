#include "Events.h"
#include "Settings.h"
bool teste = false;
//void AttachTriggerToActor(RE::Actor* actor) {
//    if (teste) return;
//
//    // 1. Localiza a base do Activator
//    RE::TESObjectACTI* triggerBase = Sink::test1;
//    auto player = RE::PlayerCharacter::GetSingleton();
//
//    if (!triggerBase || !player) return;
//
//
//    auto spawnedRef = actor->PlaceObjectAtMe(triggerBase, false);
//    spawnedRef->SetMotionType(RE::hkpMotion::MotionType::kKeyframed, true);
//        auto trigger3D = spawnedRef->Load3D(true);
//            RE::BSFixedString nodeName = "WEAPON"; 
//            auto playerNode = player->Load3D(true)->AsNode();
//            auto targetNode = playerNode->GetObjectByName(nodeName);
//
//                targetNode->AsNode()->AttachChild(trigger3D, true);
//
//                RE::NiUpdateData ctx;
//                trigger3D->Update(ctx);
//
//                logger::info("Trigger 3D anexado ao node {} do player", nodeName.c_str());
//
//        
//    
//    teste = true;
//}

RE::BSEventNotifyControl Sink::NpcCombatTracker::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*)
{
    if (!a_event || !a_event->actor) {
        return RE::BSEventNotifyControl::kContinue;
    }
    
    auto actor = a_event->actor.get();
    auto* npc = actor->As<RE::Actor>();
    if (npc && npc != RE::PlayerCharacter::GetSingleton()) {  // Garante que é um ator válido
        switch (a_event->newState.get()) {
        case RE::ACTOR_COMBAT_STATE::kCombat:
            NpcCombatTracker::RegisterSink(npc);
            //AttachTriggerToActor(npc);
            break;
        case RE::ACTOR_COMBAT_STATE::kNone:
            NpcCombatTracker::UnregisterSink(npc);
            break;
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

void Sink::NpcCombatTracker::RegisterSink(RE::Actor* a_actor)
{
    std::unique_lock lock(g_mutex);
    if (g_trackedNPCs.find(a_actor->GetFormID()) == g_trackedNPCs.end()) {
        a_actor->AddAnimationGraphEventSink(&g_npcSink);
        g_trackedNPCs.insert(a_actor->GetFormID());
        //SKSE::log::info("[NpcCombatTracker] Começando a rastrear animações do ator {:08X}", a_actor->GetFormID());
    }
}

void Sink::NpcCombatTracker::UnregisterSink(RE::Actor* a_actor)
{
    if (!a_actor || a_actor->IsPlayerRef()) return;

    std::unique_lock lock(g_mutex);
    if (g_trackedNPCs.find(a_actor->GetFormID()) != g_trackedNPCs.end()) {
        a_actor->RemoveAnimationGraphEventSink(&g_npcSink);
        g_trackedNPCs.erase(a_actor->GetFormID());
        //SKSE::log::info("[NpcCombatTracker] Parando de rastrear animações do ator {:08X}", a_actor->GetFormID());
    }
}

void Sink::NpcCombatTracker::RegisterSinksForExistingCombatants()
{
    SKSE::log::info("[NpcCombatTracker] Verificando NPCs já em combate após carregar o jogo...");

    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) {
        SKSE::log::warn("[NpcCombatTracker] Não foi possível obter ProcessLists.");
        return;
    }

    // Itera sobre todos os atores que estão "ativos" no jogo
    for (auto& actorHandle : processLists->highActorHandles) {
        if (auto actor = actorHandle.get().get()) {
            // A função IsInCombat() nos diz se o ator já está em um estado de combate
            if (!actor->IsPlayerRef()) {
                if (actor->IsInCombat()) {
                    SKSE::log::info("[NpcCombatTracker] Ator '{}' ({:08X}) já está em combate. Registrando sink...",
                        actor->GetName(), actor->GetFormID());
                    // Usamos a mesma função de registro que já existe!
                    RegisterSink(actor);
                }
            }

        }
    }

    SKSE::log::info("[NpcCombatTracker] Verificação concluída.");
}

bool IsPlayerInDanger(RE::Actor* npc, RE::PlayerCharacter* player) {
    if (!npc->IsAttacking()) return false;

    // 1. Verificar distância (alcance da arma)
    float distance = npc->GetDistance(player);
    if (distance > 250.0f) return false; // Exemplo de alcance melee
    RE::NiPoint3 origin;
    RE::NiPoint3 forward; // O parâmetro a_direction será preenchido aqui

    // False indica que não queremos o offset da câmera (ideal para NPCs)
    npc->GetEyeVector(origin, forward, false);
    RE::NiPoint3 toPlayer = player->GetPosition() - npc->GetPosition();
    toPlayer.Unitize();

    float cosAngle = forward.Dot(toPlayer);
    return cosAngle > 0.7f;
}

RE::BSEventNotifyControl Sink::NpcCycleSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
{
    if (!a_event || !a_event->holder) return RE::BSEventNotifyControl::kContinue;

    auto* actor = a_event->holder->As<RE::Actor>();
    if (!actor || actor->IsDead() || actor->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;

    const RE::FormID formID = actor->GetFormID();
    const std::string_view eventName = a_event->tag;

    auto npc = const_cast<RE::Actor*>(actor);
    int isPower = actor->IsPowerAttacking();
	bool isUnblockable = false;
	bool didMath = false;
    npc->GetGraphVariableBool("UnblockableAttackCalcCMF", didMath);
	auto player = RE::PlayerCharacter::GetSingleton();
    if (!isUnblockable && !didMath) {
        bool shouldTrigger = false;
        for (const auto& targetEvent : UnblockableSettings::triggerEvents) {
            if (eventName == targetEvent) {
                shouldTrigger = true;
                break;
            }
        }

        if (shouldTrigger) {
            npc->SetGraphVariableBool("UnblockableAttackCalcCMF", true);
            if (UnblockableManager::CalculateUnblockableChance(npc, isPower)) {   
                npc->NotifyAnimationGraph("UnblockableHitStartCMF");
                {
                    std::unique_lock lock(UnblockableManager::g_unblockableMutex);
                    UnblockableManager::g_unblockableStatus[formID] = true;
                }
                if (IsPlayerInDanger(npc,player)) {
                    auto& settings = isPower ? UnblockableSettings::powerAttacks : UnblockableSettings::normalAttacks;
                    if (settings.slowTimeEnabled) {
                        ApplySlowTime(settings.slowTimeDuration, settings.slowTimeMultiplier);
                    }
                }

                UnblockableManager::PlayUnblockableVisuals(npc, isPower);
            }
        }
    }    
    else if (eventName == "attackStop" || eventName == "CastOKStop") {
        npc->SetGraphVariableBool("UnblockableAttackCalcCMF", false);
        npc->NotifyAnimationGraph("UnblockableHitEndCMF");
        {
            std::unique_lock lock(UnblockableManager::g_unblockableMutex);
            UnblockableManager::g_unblockableStatus[formID] = false;
        }
    }
       
    return RE::BSEventNotifyControl::kContinue;
}

RE::ActorValue UnblockableManager::GetSkillForWeapon(RE::TESObjectWEAP* a_weapon)
{
    if (!a_weapon) return RE::ActorValue::kNone;

    // Acessando via animationType conforme a estrutura DNAM fornecida
    switch (a_weapon->weaponData.animationType.get()) {
    case RE::WEAPON_TYPE::kOneHandSword:
    case RE::WEAPON_TYPE::kOneHandDagger:
    case RE::WEAPON_TYPE::kOneHandAxe:
    case RE::WEAPON_TYPE::kOneHandMace:
    case RE::WEAPON_TYPE::kStaff: // Cajados geralmente usam animação de uma mão
        return RE::ActorValue::kOneHanded;

    case RE::WEAPON_TYPE::kTwoHandSword:
    case RE::WEAPON_TYPE::kTwoHandAxe:
        return RE::ActorValue::kTwoHanded;

    case RE::WEAPON_TYPE::kBow:
    case RE::WEAPON_TYPE::kCrossbow:
        return RE::ActorValue::kArchery;

    case RE::WEAPON_TYPE::kHandToHandMelee:
        // Em Skyrim, NPCs usam OneHanded para cálculos de combate desarmado frequentemente
        return RE::ActorValue::kOneHanded;

    default:
        return RE::ActorValue::kNone;
    }
}

void UnblockableManager::PlayUnblockableVisuals(RE::Actor* a_actor,bool isPower)
{
    if (!a_actor) return;
    // Busca a configuração correta (Normal ou Power) baseada no ataque
    auto& settings = isPower ? UnblockableSettings::powerAttacks : UnblockableSettings::normalAttacks;

    // --- Lógica de Som ---
    if (settings.soundEnabled) {
        auto sound = isPower ? Sink::UnblockHitPowerSound : Sink::UnblockHitSound;
        a_actor->ApplyEffectShader(sound, 1.5f, nullptr, false, false);

    }

    // --- Lógica Visual ---
    if (settings.visualsEnabled) {
        auto actor3D = a_actor->Get3D();
        if (!actor3D) return;

        RE::NiAVObject* targetNode = actor3D->GetObjectByName("WEAPON");
        if (!targetNode) {
            targetNode = actor3D->GetObjectByName("NPC R Hand [RHand]");
        }

        if (isPower) {
            a_actor->ApplyArtObject(Sink::UnblockPowerHit, 5.0f, nullptr, false, false, targetNode);
        }
        else {
            a_actor->ApplyArtObject(Sink::UnblockHit, 5.0f, nullptr, false, false, targetNode);
        }

        //SKSE::log::info("Visual de ataque aplicado ao node: {}", targetNode ? targetNode->name.c_str() : "Root");
    }
    if (settings.effectShaderEnabled) {
        auto shader = isPower ? Sink::ShaUnblockPowerHit : Sink::ShaUnblockNormalHit;
        if (shader) {
            // Aplica o shader ao ator. 
            a_actor->ApplyEffectShader(shader, settings.effectShaderDuration, nullptr, false, false);
        }
    }

}


bool UnblockableManager::CalculateUnblockableChance(RE::Actor* a_actor, bool isPower)
{
    if (!a_actor) return false;

    auto& settings = isPower ? UnblockableSettings::powerAttacks : UnblockableSettings::normalAttacks;
    if (!settings.enabled) return false;



    float healthPct = std::clamp(a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) /
        a_actor->GetActorValueMax(RE::ActorValue::kHealth), 0.0f, 1.0f);
    float healthWeight = (1.0f - healthPct) * settings.healthMult;

    float aggression = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kAggression);
	//logger::info("[Unblockable] Ator {:08X} Aggression: {:.2f}", a_actor->GetFormID(), aggression);
    float aggressionWeight = aggression * settings.aggressionMult;

    float skillWeight = 0.0f;
    if (auto weapon = a_actor->GetAttackingWeapon()) {
        if (auto weaponData = weapon->object->As<RE::TESObjectWEAP>()) {
            RE::ActorValue skillAV = GetSkillForWeapon(weaponData);
            float skillValue = a_actor->AsActorValueOwner()->GetActorValue(skillAV);
            skillWeight = skillValue * settings.skillMult;
        }
    }


    float attackerPower = settings.baseChance + healthWeight + aggressionWeight + skillWeight;
    float resistance = settings.globalDifficulty;
    float finalChance = attackerPower / (attackerPower + resistance);


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    float roll = static_cast<float>(dis(gen));

    if (roll < finalChance) {
        //logger::info("[Unblockable] Sucesso! Power: {:.2f}, Resist: {:.2f}, Chance Final: {:.2f}%",attackerPower, resistance, finalChance * 100.0f);
        return true;
    }
    //logger::info("[Unblockable] Falha. Power: {:.2f}, Resist: {:.2f}, Chance Final: {:.2f}%",attackerPower, resistance, finalChance * 100.0f);
    return false;
}

void Sink::ApplySlowTime(int a_duration, float a_multiplier)
{
    if (RE::BSTimer::QGlobalTimeMultiplier() != 1.0f) {
        return;
    }
    auto* timer = RE::BSTimer::GetSingleton();
    if (timer) {
        timer->SetGlobalTimeMultiplier(a_multiplier, true);
        int durationMs = static_cast<int>(a_duration * 1000.0f);
        g_IsSlowed = true;
        std::thread([a_duration]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(a_duration));

            // Retorna para a thread principal do SKSE para evitar instabilidade
            SKSE::GetTaskInterface()->AddTask([]() {
                auto* timer = RE::BSTimer::GetSingleton();
                if (timer) {
                    timer->SetGlobalTimeMultiplier(1.0f, true);
                    g_IsSlowed = false;
                }
                });
            }).detach();
    
    
    }
}

void Sink::InitializeForms() {
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;


    UnblockHit = dataHandler->LookupForm<RE::BGSArtObject>(0x803, "Unblockable.esp");
    UnblockPowerHit = dataHandler->LookupForm<RE::BGSArtObject>(0x802, "Unblockable.esp");

	UnblockHitSound = dataHandler->LookupForm<RE::TESEffectShader>(0x80B, "Unblockable.esp");
    UnblockHitPowerSound = dataHandler->LookupForm<RE::TESEffectShader>(0x80C, "Unblockable.esp");

    ShaUnblockNormalHit = dataHandler->LookupForm<RE::TESEffectShader>(0x805, "Unblockable.esp");
    ShaUnblockPowerHit = dataHandler->LookupForm<RE::TESEffectShader>(0x80A, "Unblockable.esp");

    //test1 = dataHandler->LookupForm<RE::TESObjectACTI>(0x909, "Unblockable.esp");

    if (!UnblockHit) {
        SKSE::log::critical("FALHA: não encontrado em UnblockHit.esp!");
    }
    else {
        SKSE::log::info("UnblockHit carregado com sucesso.");
    }

}

void Sink::ScheduleSinkRegistration(RE::Actor* actor, int attempts)
{
    if (attempts > 20) {
        SKSE::log::critical("[Actor3DLoadEventHandler] Desistindo após {} tentativas para o ator {:08X}.", attempts, actor->GetFormID());
        return;
    }

    std::thread([actor, attempts]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        SKSE::GetTaskInterface()->AddTask([actor, attempts]() {
            if (!actor) return;

            RE::BSTSmartPointer<RE::BSAnimationGraphManager> graphManager;
            actor->GetAnimationGraphManager(graphManager);

            if (graphManager) {
                SKSE::log::info("[Actor3DLoadEventHandler] Graph encontrado para {:08X}. Reconectando...", actor->GetFormID());

                if (!actor->IsPlayerRef()) {
                    Sink::NpcCombatTracker::UnregisterSink(actor);
                    Sink::NpcCombatTracker::RegisterSink(actor);
                    SKSE::log::info("[Actor3DLoadEventHandler] Sink de NPC reconectada (via CombatTracker).");

                }
            }
            else {
                // Graph ainda nulo, tenta de novo
                ScheduleSinkRegistration(actor, attempts + 1);
            }
            });
        }).detach();
}

RE::BSEventNotifyControl Sink::PC3DLoadEventHandler::ProcessEvent(const RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
{
    if (!a_event || !a_event->loaded) {
        return RE::BSEventNotifyControl::kContinue;
    }
    auto* form = RE::TESForm::LookupByID(a_event->formID);
    if (!form) return RE::BSEventNotifyControl::kContinue;
    auto* actor = form->As<RE::Actor>();

    if (actor) {
        ScheduleSinkRegistration(actor, 0);
    }

    return RE::BSEventNotifyControl::kContinue;
}

