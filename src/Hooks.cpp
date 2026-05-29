#include "Hooks.h"
#include "Settings.h"
#include "Serialization.h"

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

	bool isPower = aggressor->IsPowerAttacking();
	auto settings = UnblockableSettings::GetSettingsForActor(aggressor, isPower);

	bool isUnblockable = false;
	aggressor->GetGraphVariableBool("isUnblockableHit", isUnblockable);
	bool hasImunity = false;
	victim->GetGraphVariableBool("hasStaggerImunityCMF", hasImunity);
	bool isDodging = false;
	victim->GetGraphVariableBool("isDodgingCMF", isDodging);
	bool hasIframe = false;
	victim->GetGraphVariableBool("hasIframeCMF", hasIframe);

	if (hasIframe || isDodging) {
		_ProcessHit(victim, hitData);
		return;
	}
	else if (isUnblockable) {
		if (hitData.flags.any(RE::HitData::Flag::kBlocked)) {
			hitData.flags.reset(RE::HitData::Flag::kBlocked);
			hitData.percentBlocked = 0.0f;
			victim->NotifyAnimationGraph("blockStop");
		}
		if (settings.staggerEnabled && !hasImunity) {
			ApplyStagger(victim, settings.staggerMagnitude);
		}
		victim->NotifyAnimationGraph("HitByUnblockAtk");
		aggressor->NotifyAnimationGraph("UnblockableHitCMF");
		if (victim->IsPlayerRef()) {
			Tracking::UnblockableHitsSTM++;
			victim->SetGraphVariableInt("UnblockableHitsSTM", static_cast<int>(Tracking::UnblockableHitsSTM));
		}
	}
	_ProcessHit(victim, hitData);
}

bool processProjectileBlock(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable)
{
	auto shooterHandle = a_projectile->GetProjectileRuntimeData().shooter;
	auto shooter = shooterHandle.get().get() ? shooterHandle.get().get()->As<RE::Actor>() : nullptr;
	if (!shooter) return false;

	bool isPower = shooter->IsPowerAttacking();
	auto settings = UnblockableSettings::GetSettingsForActor(shooter, isPower);

	bool isUnblockable = false;
	shooter->GetGraphVariableBool("isUnblockableHit", isUnblockable);
	bool hasImunity = false;
	a_blocker->GetGraphVariableBool("hasStaggerImunityCMF", hasImunity);
	bool isDodging = false;
	a_blocker->GetGraphVariableBool("isDodgingCMF", isDodging);
	bool hasIframe = false;
	a_blocker->GetGraphVariableBool("hasIframeCMF", hasIframe);

	if (hasIframe || isDodging) {
		return true;
	}
	else if (isUnblockable) {
		if (a_blocker->IsBlocking()) {
			a_blocker->NotifyAnimationGraph("blockStop");
		}
		if (settings.staggerEnabled && !hasImunity) {
			ApplyStagger(a_blocker, settings.staggerMagnitude);
		}
		a_blocker->NotifyAnimationGraph("HitByUnblockAtk");
		shooter->NotifyAnimationGraph("UnblockableHitCMF");
		if (a_blocker->IsPlayerRef()) {
			Tracking::UnblockableHitsSTM++;
			a_blocker->SetGraphVariableInt("UnblockableHitsSTM", static_cast<int>(Tracking::UnblockableHitsSTM));
		}
	}
	return false;
}

inline bool shouldIgnoreHit(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
{
	if (a_AllCdPointCollector) {
		for (auto& hit : a_AllCdPointCollector->hits) {
			auto refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
			auto refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);
			RE::Actor* target = nullptr;
			RE::hkpCollidable* projectileCollidable = nullptr;

			if (refrA && refrA->formType == RE::FormType::ActorCharacter) {
				target = refrA->As<RE::Actor>();
				projectileCollidable = const_cast<RE::hkpCollidable*>(hit.rootCollidableB);
			}
			else if (refrB && refrB->formType == RE::FormType::ActorCharacter) {
				target = refrB->As<RE::Actor>();
				projectileCollidable = const_cast<RE::hkpCollidable*>(hit.rootCollidableA);
			}

			if (target) {
				if (processProjectileBlock(target, a_projectile, projectileCollidable)) {
					return true;
				}
			}
			else {
			}
		}
	}
	return false;
}

void Hook_OnProjectileCollision::OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
{
	if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
		return;
	}
	_arrowCollission(a_this, a_AllCdPointCollector);
}

void Hook_OnProjectileCollision::OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
{
	if (a_this && (a_this->GetProjectileRuntimeData().spell || a_this->GetProjectileBase())) {
		if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
			return;
		}
	}
	_missileCollission(a_this, a_AllCdPointCollector);
}

