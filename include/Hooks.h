#pragma once
#include "Events.h"

class Hook_OnMeleeHit
{
public:
	static void install()
	{
		auto& trampoline = SKSE::GetTrampoline();
		constexpr size_t size_per_hook = 14;
		constexpr size_t NUM_TRAMPOLINE_HOOKS = 1;
		trampoline.create(size_per_hook * NUM_TRAMPOLINE_HOOKS);
		REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37673, 38627) };  
		_ProcessHit = trampoline.write_call<5>(hook.address() + REL::Relocate(0x3C0, 0x4A8), processHit);
		logger::info("hook:OnMeleeHit");
	}

private:
	static void processHit(RE::Actor* victim, RE::HitData& hitData);
	static inline REL::Relocation<decltype(processHit)> _ProcessHit;  
};

