#include "logger.h"
#include "Hooks.h"
#include "Settings.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Sink::InitializeForms();
        Hook_OnMeleeHit::install();
        UnblockableSettings::UnBlockLoad();
        UnblockableSettings::UnBlockRegister();
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(Sink::PC3DLoadEventHandler::GetSingleton());
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(Sink::NpcCombatTracker::GetSingleton());
        Sink::NpcCombatTracker::RegisterSinksForExistingCombatants();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
