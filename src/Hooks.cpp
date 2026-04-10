#include "Hooks.h"
#include "Settings.h"

void ApplyStagger(RE::Actor* a_target, float a_magnitude) {
	if (!a_target) return;
	a_target->SetGraphVariableFloat("staggerMagnitude", a_magnitude);
	a_target->NotifyAnimationGraph("staggerStart");
}

void Hook_OnMeleeHit::processHit(RE::Actor* victim, RE::HitData& hitData)
{
	auto aggressor = hitData.aggressor.get().get();
	if (!victim || !aggressor) {
		_ProcessHit(victim, hitData);
		return;
	}
    auto& settings = aggressor->IsPowerAttacking() ?
        UnblockableSettings::powerAttacks :
        UnblockableSettings::normalAttacks;
    bool isUnblockable = false;
    {
        std::shared_lock lock(UnblockableManager::g_unblockableMutex);
        auto it = UnblockableManager::g_unblockableStatus.find(aggressor->GetFormID());
        if (it != UnblockableManager::g_unblockableStatus.end()) {
            isUnblockable = it->second;
        }
    }

    if (isUnblockable) {
        victim->NotifyAnimationGraph("HitByUnblockAtk");
        aggressor->NotifyAnimationGraph("UnblockableHitCMF");
        if (hitData.flags.any(RE::HitData::Flag::kBlocked)) {
            hitData.flags.reset(RE::HitData::Flag::kBlocked);
            hitData.percentBlocked = 0.0f;
			victim->NotifyAnimationGraph("blockStop");
            //logger::info("[Combat] Ataque de {} quebrou a defesa de {}", aggressor->GetName(), victim->GetName());
        }
        if (settings.staggerEnabled) {
            ApplyStagger(victim,0.5f);
        }
        
    }
    _ProcessHit(victim, hitData);
    
}

