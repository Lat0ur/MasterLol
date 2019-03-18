#include "GameEvent.h"
#include <chrono>
#include <iostream>
#include <thread>

#include <SDK/EventManager.h>
#include <SDK/SpellInclude.h>
#include <SDK/Game.h>

#include "ObjectHelper.h"

namespace Common
{
	void OnUpdate::Run(unsigned long long updateFrequency)
	{
		using namespace std::chrono;
		static auto updateFreq = std::chrono::milliseconds(updateFrequency);
		static auto next = steady_clock::now();
		static auto prev = next - updateFreq;

		auto now = steady_clock::now();
		prev = now;

		EventHandler<EventIndex::OnUpdate, EventDefines::OnMainLoop>::GetInstance()->Trigger();

		next += updateFreq;
		std::this_thread::sleep_until(next);
	}




	std::unordered_map<MissileClient*, int> OnMissileProcessSpell::mActiveMissileMap;
	void OnMissileProcessSpell::AddMissile(MissileClient * missile)
	{
		std::pair<MissileClient*, int> newMissile(missile, missile->GetCreatedTimeMs());
		mActiveMissileMap.insert(newMissile);
	}

	void OnMissileProcessSpell::OnUpdate()
	{
		auto allMissile = ObjectList::mAllMissiles;
		for (auto missile : allMissile)
		{
			auto got = mActiveMissileMap.find(missile);
			if (got == mActiveMissileMap.end())
			{
				// Not Found
				AddMissile(missile);

				auto caster = (Obj_AI_Base*)ObjectHelper::GetSourceObject(missile);
				EventHandler<EventIndex::OnMissileProcessSpell, EventDefines::OnMissileProcessSpell,
					MissileClient*, GameObject*>::GetInstance()->Trigger(missile, caster);
				return;
			}
			if (got->second < missile->GetCreatedTimeMs())
			{
				got->second = missile->GetCreatedTimeMs();
				auto caster = (Obj_AI_Base*)ObjectHelper::GetSourceObject(missile);
				EventHandler<EventIndex::OnMissileProcessSpell, EventDefines::OnMissileProcessSpell,
					MissileClient*, GameObject*>::GetInstance()->Trigger(missile, caster);
				return;
			}
		}
	}

	std::unordered_map<UINT16, SpellCastInfo> OnProcessSpell::mActiveProcessSpell;
	void OnProcessSpell::OnUpdate()
	{
		auto allHero = ObjectList::mAllHeros;
		for (Obj_AI_Base* hero : allHero)
		{
			Spellbook* spellbook = hero->GetSpellbook();
			if (!spellbook) continue;

			SpellCastInfo* activeSpell = spellbook->GetActiveSpell();
			if (!activeSpell) continue;

			auto got = mActiveProcessSpell.find(activeSpell->mMissileIndex);
			if (got != mActiveProcessSpell.end())
				continue;

			std::pair<UINT16, SpellCastInfo> newProcessSpell(activeSpell->mMissileIndex, *activeSpell);

			printf("ActiveProcessSpell :\n\
					\tIndex : %d, Hero : %s\n",
					activeSpell->mMissileIndex, hero->GetAIName().c_str());

			mActiveProcessSpell.insert(newProcessSpell);

			EventHandler<EventIndex::OnProcessSpell, EventDefines::OnProcessSpell,
				SpellCastInfo, Obj_AI_Base*>::GetInstance()->Trigger(*activeSpell, hero);
		}

		for (auto it = mActiveProcessSpell.begin(); it != mActiveProcessSpell.end(); ) {
			if (Game::GetGameTime() > it->second.mEndTime) {
				printf("erase spell\n");
				it = mActiveProcessSpell.erase(it);
			}
			else
				it++;
		}

	}
}

