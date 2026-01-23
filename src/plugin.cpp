#include "logger.h"
#include "Hooks.h"
#include "Settings.h"
auto player = RE::PlayerCharacter::GetSingleton();
//void AttachTriggerToActor(RE::Actor* a_actor) {
//
//    // 1. Localiza a base do Activator (TriggerVolume)
//    auto triggerBase = Sink::test1;
//
//    // 2. Coloca a referência no mundo na posição do ator
//    auto cell = a_actor->GetParentCell();
//    auto triggerRef = a_actor->PlaceObjectAtMe(triggerBase, true);
//	logger::info("TriggerVolume colocado no ator {:08X}", a_actor->GetFormID());
//    //if (triggerRef) {
//    //    // 3. Atacha o nó 3D para que o trigger siga o ator automaticamente pelo motor gráfico
//    //    auto actorNode = a_actor->Get3D();
//    //    auto triggerNode = triggerRef->Get3D();
//
//    //    if (actorNode && triggerNode) {
//    //        actorNode->AsNode()->AttachChild(triggerNode);
//
//    //        // Atualiza o gráfico de cena para aplicar a mudança
//    //        RE::NiUpdateData updateData;
//    //        triggerNode->Update(updateData);
//
//    //        SKSE::log::info("Activator anexado ao actor {:08X}", a_actor->GetFormID());
//    //    }
//
//    //    
//    //}
//}

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
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESTriggerEvent>(MyTriggerSink2::GetSingleton());
        auto eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventHolder) {
            eventHolder->AddEventSink<RE::TESTriggerEnterEvent>(MyTriggerSink::GetSingleton());
           
            SKSE::log::info("Sink de evento registrado com sucesso.");
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
