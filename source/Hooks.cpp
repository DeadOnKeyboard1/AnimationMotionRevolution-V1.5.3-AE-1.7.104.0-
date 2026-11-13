#include "Hooks.h"

#include "AnimMotionHandler.h"
#include "CharacterClipAnimMotionMap.h"

#include "utils/Logger.h"
#include "MotionDiagnostics.h"

namespace hooks
{
	// Called when animations are activated by clip generators
	std::uint32_t hkbClipGenerator::ComputeStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_hkbContext)
	{
		if (!a_this) return ComputeStartTime(a_this);
#ifdef AMR_MOTION_DIAGNOSTICS
		amr::diagnostics::Trace trace{amr::diagnostics::Trace::Kind::activation};
		trace.SetClip(a_this->name.c_str());
		if (trace.capturing) trace.animation = amr::diagnostics::Label(a_this->animationName.c_str() ? a_this->animationName.c_str() : "");
		trace.graph = a_hkbContext ? a_hkbContext->character : nullptr;
#endif
		auto characterClipAnimMotionMap = CharacterClipAnimMotionMap::GetSingleton();

		RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

		const RE::hkaAnimation* boundAnimation = GetBoundAnimation(a_this);
#ifdef AMR_MOTION_DIAGNOSTICS
		if (trace.capturing) {
			trace.bound = boundAnimation != nullptr;
			trace.tracks = boundAnimation ? boundAnimation->annotationTracks.size() : 0;
		}
#endif

		if (boundAnimation)
		{
			const RE::hkbCharacter* hkbCharacter = a_hkbContext ? a_hkbContext->character : nullptr;

			if (hkbCharacter)
			{
				AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, a_this->name);

				// Animation activation is multithreaded, therefore I need to keep track of the number of times
				// they are activated
				if (animMotionData && animMotionData->animation == boundAnimation)
				{
					if (!animMotionData->Activate()) {
						characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
					}
				}
				else
				{
					if (animMotionData) {
						characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
						animMotionData = nullptr;
					}
					Translation* translation = nullptr;
					Rotation* rotation = nullptr;
					bool cacheUnavailable = false;

					try {
					for (const RE::hkaAnnotationTrack& annotationTrack : boundAnimation->annotationTracks)
					{
						for (const RE::hkaAnnotationTrack::Annotation& annotation : annotationTrack.annotations)
						{
#ifdef AMR_MOTION_DIAGNOSTICS
							if (trace.capturing) ++trace.annotations;
#endif
							auto dataParsed = ParseAnnotation(annotation);

							translation = std::get_if<Translation>(&dataParsed);

							if (translation)
							{
								if (animMotionData && animMotionData->animation == boundAnimation)
								{
									animMotionData->Add(translation);
								}
								else
								{
									animMotionData = characterClipAnimMotionMap->Add(hkbCharacter, a_this->name, boundAnimation, translation);
									if (!animMotionData) { cacheUnavailable = true; break; }
								}
							}
							else
							{
								rotation = std::get_if<Rotation>(&dataParsed);

								if (rotation)
								{
									if (animMotionData && animMotionData->animation == boundAnimation)
									{
										animMotionData->Add(rotation);
									}
									else
									{
										animMotionData = characterClipAnimMotionMap->Add(hkbCharacter, a_this->name, boundAnimation, rotation);
										if (!animMotionData) { cacheUnavailable = true; break; }
									}
								}
							}
						}

						if (cacheUnavailable) break;
						if (animMotionData)
						{
							animMotionData->SortListsByTime();

							if (!animMotionData->translationList.empty() && animMotionData->translationList.back().time != boundAnimation->duration)
							{
								logger::warn("Animation={} of hkbCharacter=0x{:08x} ends at {}, while custom translation ends at {}",
									a_this->animationName.c_str() ? a_this->animationName.c_str() : "<unnamed>", reinterpret_cast<std::uint64_t>(hkbCharacter),
									boundAnimation->duration, animMotionData->translationList.back().time);
							}

							if (!animMotionData->rotationList.empty() && animMotionData->rotationList.back().time != boundAnimation->duration)
							{
								logger::warn("Animation={} of hkbCharacter=0x{:08x} ends at {}, while custom rotation ends at {}",
									a_this->animationName.c_str() ? a_this->animationName.c_str() : "<unnamed>", reinterpret_cast<std::uint64_t>(hkbCharacter),
									boundAnimation->duration, animMotionData->rotationList.back().time);
							}

							// Support only for annotations in the same track, quit the loop when found
							break;
						}
					}
					} catch (const std::bad_alloc&) {
						characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
						logger::warn("Custom motion allocation failed; retaining native motion");
					} catch (const std::length_error&) {
						characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
						logger::warn("Custom motion list exceeded its size limit; retaining native motion");
					}
				}
			}
		}

#ifdef AMR_MOTION_DIAGNOSTICS
		if (trace.enabled) {
			if (const auto data = characterClipAnimMotionMap->Get(trace.graph, a_this->name)) {
				trace.cacheHit = true;
				trace.translations = data->translationList.size();
				trace.rotations = data->rotationList.size();
				trace.active = data->activeCount;
			}
		}
#endif
		return ComputeStartTime(a_this);
	}

	// Called when animations are deactivated by clip generators
	void hkbClipGenerator::ResetIgnoreStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_hkbContext)
	{
		if (!a_this) { ResetIgnoreStartTime(a_this); return; }
#ifdef AMR_MOTION_DIAGNOSTICS
		amr::diagnostics::Trace trace{amr::diagnostics::Trace::Kind::deactivation};
		trace.SetClip(a_this->name.c_str());
		trace.graph = a_hkbContext ? a_hkbContext->character : nullptr;
#endif
		auto characterClipAnimMotionMap = CharacterClipAnimMotionMap::GetSingleton();
		RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

		const RE::hkaAnimation* boundAnimation = GetBoundAnimation(a_this);

		if (boundAnimation)
		{
			const RE::hkbCharacter* hkbCharacter = a_hkbContext ? a_hkbContext->character : nullptr;

			if (hkbCharacter)
			{
				AnimMotionData* animMotionData = characterClipAnimMotionMap->Get(hkbCharacter, a_this->name);

				// Animation deactivation is also multithreaded, so keep track of the number of
				// activated times left
				if (animMotionData && animMotionData->animation == boundAnimation)
				{
#ifdef AMR_MOTION_DIAGNOSTICS
					if (trace.capturing) {
						trace.bound = true;
						trace.cacheHit = true;
						trace.translations = animMotionData->translationList.size();
						trace.rotations = animMotionData->rotationList.size();
						trace.active = animMotionData->activeCount;
					}
#endif
					if (animMotionData->Deactivate())
					{
						characterClipAnimMotionMap->Remove(hkbCharacter, a_this->name);
					}
				}
			}
		}

		ResetIgnoreStartTime(a_this);
	}

	void MotionDataContainer::ProcessTranslationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime,
														  RE::NiPoint3& a_translation, const RE::BSFixedString* a_clipName,
														  RE::Character* a_character)
	{
#ifdef AMR_MOTION_DIAGNOSTICS
		amr::diagnostics::Trace trace{amr::diagnostics::Trace::Kind::translation};
		trace.SetClip(a_clipName ? a_clipName->c_str() : nullptr);
		trace.time = a_motionTime;
#endif
		auto characterClipAnimMotionMap = CharacterClipAnimMotionMap::GetSingleton();
		RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

		RE::hkbCharacter* hkbCharacter = GethkbCharacter(a_character);

		AnimMotionData* animMotionData = a_clipName ? characterClipAnimMotionMap->Get(hkbCharacter, *a_clipName) : nullptr;

		std::vector<Translation>* customTranslationList = animMotionData ? &animMotionData->translationList : nullptr;

		bool hasCustomMotionList = customTranslationList && !customTranslationList->empty();
#ifdef AMR_MOTION_DIAGNOSTICS
		trace.SetPlayer(a_character, hkbCharacter);
		if (trace.capturing) {
			trace.cacheHit = animMotionData != nullptr;
			trace.custom = hasCustomMotionList;
			trace.translations = animMotionData ? animMotionData->translationList.size() : 0;
			trace.rotations = animMotionData ? animMotionData->rotationList.size() : 0;
		}
#endif

		if (hasCustomMotionList)
		{
			float endMotionTime = customTranslationList->back().time;

			float curMotionTime = (a_motionTime > endMotionTime) ? endMotionTime : a_motionTime;

			const auto segCount = customTranslationList->size();

			for (std::size_t segIndex = 1; segIndex <= segCount; segIndex++)
			{
				float curSegMotionTime = customTranslationList->at(segIndex - 1).time;

				if (curMotionTime <= curSegMotionTime)
				{
					const auto prevSegIndex = segIndex - 1;
					float segProgress = 1.0f;

					float prevSegMotionTime = prevSegIndex ? customTranslationList->at(prevSegIndex - 1).time : 0.0f;

					float curSegMotionDuration = curSegMotionTime - prevSegMotionTime;
					if (curSegMotionDuration > std::numeric_limits<float>::epsilon())
					{
						segProgress = (curMotionTime - prevSegMotionTime) / curSegMotionDuration;
					}

					const RE::NiPoint3& curSegTranslation = customTranslationList->at(segIndex - 1).delta;

					const RE::NiPoint3& prevSegTranslation = prevSegIndex ?
																   customTranslationList->at(prevSegIndex - 1).delta :
																   RE::NiPoint3{ 0.0f, 0.0f, 0.0f };

					a_translation = (curSegTranslation * segProgress + prevSegTranslation * (1.0f - segProgress));
#ifdef AMR_MOTION_DIAGNOSTICS
					trace.output = {a_translation.x, a_translation.y, a_translation.z, 0};
#endif

					return;
				}
			}
		}
		else
		{
			// The game checks this in the original code, so we do
			bool hasVanillaMotionList = a_this->translationSegCount > static_cast<std::uint32_t>(a_this->IsTranslationDataAligned());

			if (hasVanillaMotionList)
			{
				ProcessTranslationData(&a_this->translationDataPtr, a_motionTime, a_translation);
#ifdef AMR_MOTION_DIAGNOSTICS
				trace.output = {a_translation.x, a_translation.y, a_translation.z, 0};
#endif

				return;
			}
		}

		a_translation = RE::NiPoint3{ 0.0f, 0.0f, 0.0f };
	}

	void MotionDataContainer::ProcessRotationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime,
													   RE::NiQuaternion& a_rotation, const RE::BSFixedString* a_clipName,
													   RE::Character* a_character)
	{
#ifdef AMR_MOTION_DIAGNOSTICS
		amr::diagnostics::Trace trace{amr::diagnostics::Trace::Kind::rotation};
		trace.SetClip(a_clipName ? a_clipName->c_str() : nullptr);
		trace.time = a_motionTime;
#endif
		auto characterClipAnimMotionMap = CharacterClipAnimMotionMap::GetSingleton();
		RE::BSSpinLockGuard lockguard(characterClipAnimMotionMap->lock);

		RE::hkbCharacter* hkbCharacter = GethkbCharacter(a_character);

		AnimMotionData* animMotionData = a_clipName ? characterClipAnimMotionMap->Get(hkbCharacter, *a_clipName) : nullptr;

		std::vector<Rotation>* customRotationList = animMotionData ? &animMotionData->rotationList : nullptr;

		bool hasCustomMotionList = customRotationList && !customRotationList->empty();
#ifdef AMR_MOTION_DIAGNOSTICS
		trace.SetPlayer(a_character, hkbCharacter);
		if (trace.capturing) {
			trace.cacheHit = animMotionData != nullptr;
			trace.custom = hasCustomMotionList;
			trace.translations = animMotionData ? animMotionData->translationList.size() : 0;
			trace.rotations = animMotionData ? animMotionData->rotationList.size() : 0;
		}
#endif

		if (hasCustomMotionList)
		{
			float endMotionTime = customRotationList->back().time;

			float curMotionTime = (a_motionTime > endMotionTime) ? endMotionTime : a_motionTime;

			for (std::size_t segIndex = 1; segIndex <= customRotationList->size(); segIndex++)
			{
				float curSegMotionTime = customRotationList->at(segIndex - 1).time;

				if (curMotionTime <= curSegMotionTime)
				{
					const auto prevSegIndex = segIndex - 1;
					float segProgress = 1.0f;

					float prevSegMotionTime = prevSegIndex ? customRotationList->at(prevSegIndex - 1).time : 0.0f;

					float curSegMotionDuration = curSegMotionTime - prevSegMotionTime;
					if (curSegMotionDuration > std::numeric_limits<float>::epsilon())
					{
						segProgress = (curMotionTime - prevSegMotionTime) / curSegMotionDuration;
					}
					const RE::NiQuaternion& curSegRotation = customRotationList->at(segIndex - 1).delta;
					const RE::NiQuaternion& prevSegRotation = prevSegIndex ?
																	customRotationList->at(prevSegIndex - 1).delta :
																	RE::NiQuaternion{ 1.0f, 0.0f, 0.0f, 0.0f };

					InterpolateRotation(a_rotation, segProgress, prevSegRotation, curSegRotation);
#ifdef AMR_MOTION_DIAGNOSTICS
					trace.output = {a_rotation.w, a_rotation.x, a_rotation.y, a_rotation.z};
#endif

					return;
				}
			}
		}
		else
		{
			// The game checks this in the original code, so we do
			bool hasVanillaMotionList = a_this->rotationSegCount > static_cast<std::uint32_t>(a_this->IsRotationDataAligned());

			if (hasVanillaMotionList)
			{
				ProcessRotationData(&a_this->rotationDataPtr, a_motionTime, a_rotation);
#ifdef AMR_MOTION_DIAGNOSTICS
				trace.output = {a_rotation.w, a_rotation.x, a_rotation.y, a_rotation.z};
#endif

				return;
			}
		}

		a_rotation = RE::NiQuaternion{ 1.0f, 0.0f, 0.0f, 0.0f };
#ifdef AMR_MOTION_DIAGNOSTICS
		trace.output = {a_rotation.w, a_rotation.x, a_rotation.y, a_rotation.z};
#endif
	}
}
