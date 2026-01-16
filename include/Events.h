#pragma once
#include <shared_mutex>
#include <chrono> 
#include <unordered_map>
#include <set>
#include <random>

namespace UnblockableManager {
    inline std::unordered_map<RE::FormID, bool> g_unblockableStatus;
    inline std::shared_mutex g_unblockableMutex;

    inline RE::ActorValue GetSkillForWeapon(RE::TESObjectWEAP* a_weapon);

    inline void PlayUnblockableVisuals(RE::Actor* a_actor,bool isPower);
    inline bool CalculateUnblockableChance(RE::Actor* a_actor, bool isPower);
}

namespace Sink {
    inline bool g_IsSlowed = false;

    inline RE::BGSArtObject* UnblockHit = nullptr;
    inline RE::BGSArtObject* UnblockPowerHit = nullptr;
    inline RE::TESEffectShader* ShaUnblockPowerHit = nullptr;
    inline RE::BGSExplosion* UnblockHitSound = nullptr;
    inline RE::BGSExplosion* UnblockHitPowerSound = nullptr;

    void ApplySlowTime(int a_duration, float a_multiplier);
     // Função auxiliar para carregar tudo no início
    void InitializeForms();

    void ScheduleSinkRegistration(RE::Actor* actor, int attempts);

    class PC3DLoadEventHandler : public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
    public:
        static PC3DLoadEventHandler* GetSingleton() {
            static PC3DLoadEventHandler singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
    };

    class NpcCycleSink : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    public:
        static NpcCycleSink* GetSingleton() {
            static NpcCycleSink singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    };

    class NpcCombatTracker : public RE::BSTEventSink<RE::TESCombatEvent> {
    public:
        static NpcCombatTracker* GetSingleton() {
            static NpcCombatTracker singleton;
            return &singleton;
        }

        // Função chamada quando um evento de combate ocorre
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event,
            RE::BSTEventSource<RE::TESCombatEvent>*) override;

        static void RegisterSink(RE::Actor* a_actor);
        static void UnregisterSink(RE::Actor* a_actor);

        static void RegisterSinksForExistingCombatants();

    private:

        inline static NpcCycleSink g_npcSink;

        inline static std::set<RE::FormID> g_trackedNPCs;
        inline static std::shared_mutex g_mutex;
    };
}


