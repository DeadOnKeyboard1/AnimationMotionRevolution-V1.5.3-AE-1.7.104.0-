#pragma once

#include "AnimMotionHandler.h"
#include "utils/Logger.h"

namespace hooks
{
	class CharacterClipAnimMotionMap
	{
	public:
		static CharacterClipAnimMotionMap* GetSingleton()
		{
			static CharacterClipAnimMotionMap instance;
			return &instance;
		}

		template <class StringT, class MotionT>
		AnimMotionData* Add(const RE::hkbCharacter* character, const StringT& name,
			const RE::hkaAnimation* animation, const MotionT* motion)
		{
			const auto text = name.c_str();
			if (!character || !text || !*text || !animation || !motion) return nullptr;
			try {
				AnimMotionData replacement{ animation, motion };
				auto& clips = data[character];
				auto [entry, inserted] = clips.insert_or_assign(std::string(text), std::move(replacement));
				return &entry->second;
			} catch (const std::exception& error) {
				data.clear();
				logger::warn("Motion cache allocation failed: {}", error.what());
				return nullptr;
			}
		}

		template <class StringT>
		AnimMotionData* Get(const RE::hkbCharacter* character, const StringT& name)
		{
			const auto text = name.c_str();
			if (!character || !text || !*text) return nullptr;
			const auto actor = data.find(character);
			if (actor == data.end()) return nullptr;
			const auto clip = actor->second.find(std::string_view(text));
			return clip == actor->second.end() ? nullptr : &clip->second;
		}

		template <class StringT>
		void Remove(const RE::hkbCharacter* character, const StringT& name)
		{
			const auto text = name.c_str();
			if (!character || !text || !*text) return;
			const auto actor = data.find(character);
			if (actor == data.end()) return;
			const auto clip = actor->second.find(std::string_view(text));
			if (clip == actor->second.end()) return;
			actor->second.erase(clip);
			if (actor->second.empty()) data.erase(actor);
		}

		mutable RE::BSSpinLock lock;

	private:
		std::map<const RE::hkbCharacter*, std::map<std::string, AnimMotionData, std::less<>>> data;
	};
}
