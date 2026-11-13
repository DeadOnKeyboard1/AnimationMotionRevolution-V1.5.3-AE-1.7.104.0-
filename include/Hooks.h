#pragma once

#include "utils/Trampoline.h"
#include "utils/HookValidation.h"
#include "utils/Logger.h"
#include "utils/CallPatchRollback.h"

#include <RE/B/BShkbAnimationGraph.h>
#include <RE/H/hkbClipGenerator.h>
#include <RE/H/hkbContext.h>

#include <RE/M/MotionDataContainer.h>

namespace hooks
{
	class hkbClipGenerator
	{
		static constexpr REL::RelocationID ActivateId{ 58602, 59252 };
		static constexpr REL::RelocationID DeactivateId{ 58604, 59254 };

	public:

		static inline REL::Relocation<std::uintptr_t> Activate;
		static inline REL::Relocation<std::uint32_t (*)(const RE::hkbClipGenerator*)> ComputeStartTime;
		static std::uint32_t ComputeStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_context);

		static inline REL::Relocation<std::uintptr_t> Deactivate;
		static inline REL::Relocation<void (*)(const RE::hkbClipGenerator*)> ResetIgnoreStartTime;
		static void ResetIgnoreStartTime_Hook(const RE::hkbClipGenerator* a_this, const RE::hkbContext* a_hkbContext);
		static void ResolveTargets()
		{
			Activate = ActivateId;
			Deactivate = DeactivateId;
		}

	private:

		static const RE::hkaAnimation* GetBoundAnimation(const RE::hkbClipGenerator* a_this) 
		{ 
			return a_this && a_this->binding ? a_this->binding->animation.get() : nullptr;
		}
	};

	class MotionDataContainer
	{
		static constexpr REL::RelocationID ProcessTranslationDataId{ 31812, 32582 };
		static constexpr REL::RelocationID ProcessRotationDataId{ 31813, 32583 };
		static constexpr REL::RelocationID InterpolateRotationId{ 69459, 70836 };

	public:

		// Translation
		static inline REL::Relocation<void (*)(std::uintptr_t*, float, RE::NiPoint3&)> ProcessTranslationData;
		static void ProcessTranslationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime, RE::NiPoint3& a_translation,
												const RE::BSFixedString* a_clipName, RE::Character* a_character);

		// Rotation
		static inline REL::Relocation<void (*)(std::uintptr_t*, float, RE::NiQuaternion&)> ProcessRotationData;
		static inline REL::Relocation<void (*)(RE::NiQuaternion&, float, const RE::NiQuaternion&, const RE::NiQuaternion&)> InterpolateRotation;
		static void ResolveTargets()
		{
			ProcessTranslationData = ProcessTranslationDataId;
			ProcessRotationData = ProcessRotationDataId;
			InterpolateRotation = InterpolateRotationId;
		}
		static void ProcessRotationData_Hook(RE::MotionDataContainer* a_this, float a_motionTime, RE::NiQuaternion& a_rotation, 
											 const RE::BSFixedString* a_clipName, RE::Character* a_character);

	private:

		static RE::hkbCharacter* GethkbCharacter(RE::Character* a_character)
		{
			if (!a_character) return nullptr;
			RE::BSAnimationGraphManagerPtr animGraph;

			a_character->GetAnimationGraphManager(animGraph);

			if (animGraph)
			{
				const auto index = animGraph->GetRuntimeData().activeGraph;
				if (index >= animGraph->graphs.size()) return nullptr;
				RE::BShkbAnimationGraphPtr activeGraph = animGraph->graphs[index];

				if (activeGraph)
				{
					return &activeGraph->characterInstance;
				}
			}

			return nullptr;
		};
	};

	class Character
	{	
		static constexpr REL::RelocationID ProcessMotionDataId{ 31949, 32703 };

	public:

		static inline REL::Relocation<std::uintptr_t> ProcessMotionData;
		static void ResolveTargets() { ProcessMotionData = ProcessMotionDataId; }
	};

	static inline std::optional<utils::ContextRegister> ValidateRuntimeLayout()
	{
		// Resolve only after the SKSE entry point has checked the selected game image.
		hkbClipGenerator::ResolveTargets();
		MotionDataContainer::ResolveTargets();
		Character::ResolveTargets();
		const auto text = REL::Module::get().segment(REL::Segment::textx);
		const auto checkedCode = [&](std::uintptr_t address, std::size_t size) -> std::span<const std::uint8_t> {
			if (address < text.address()) return {};
			const auto offset = address - text.address();
			if (offset > text.size() || size > text.size() - offset) return {};
			return { reinterpret_cast<const std::uint8_t*>(address), size };
		};
		const auto activateAddress = hkbClipGenerator::Activate.address();
		const auto deactivateAddress = hkbClipGenerator::Deactivate.address();
		const auto motionAddress = Character::ProcessMotionData.address();
		const std::array<std::size_t, 4> motionOffsets = !REL::Module::IsAE() ?
			std::array<std::size_t, 4>{0x28D, 0x2A1, 0x355, 0x368} :
			std::array<std::size_t, 4>{0x298, 0x2AA, 0x35C, 0x36D};
		if (!utils::IsCallBoundary(checkedCode(activateAddress, 0x66E + 5), 0x66E)) {
			logger::error("Unsupported animation activation hook layout; no hooks installed");
			return std::nullopt;
		}
		const auto context = utils::DeactivationContext(checkedCode(deactivateAddress, 0x1A + 5), 0x1A);
		if (!context) {
			logger::error("Unsupported animation deactivation context capture; no hooks installed");
			return std::nullopt;
		}
		for (const auto offset : motionOffsets) {
			if (!utils::IsCallBoundary(checkedCode(motionAddress, offset + 5), offset)) {
				logger::error("Unsupported character motion hook at offset 0x{:X}; no hooks installed", offset);
				return std::nullopt;
			}
		}
		logger::info("Validated hook layout for {}; deactivation context uses {}", REL::Module::get().version().string(),
			*context == utils::ContextRegister::rsi ? "RSI" : "R14");
		return context;
	}

	static inline bool Install(utils::ContextRegister context)
	{
		const auto activateAddress = hkbClipGenerator::Activate.address();
		const auto deactivateAddress = hkbClipGenerator::Deactivate.address();
		const auto motionAddress = Character::ProcessMotionData.address();
		const std::array<std::size_t, 4> motionOffsets = !REL::Module::IsAE() ?
			std::array<std::size_t, 4>{0x28D, 0x2A1, 0x355, 0x368} :
			std::array<std::size_t, 4>{0x298, 0x2AA, 0x35C, 0x36D};
		SKSE::AllocTrampoline(512);
		// Keep restoration armed until all potentially throwing load work is complete.
		utils::CallPatchRollback rollback({activateAddress + 0x66E, deactivateAddress + 0x1A,
			motionAddress + motionOffsets[0], motionAddress + motionOffsets[1],
			motionAddress + motionOffsets[2], motionAddress + motionOffsets[3]}, &REL::safe_write);

		// hkbClipGenerator::Activate
		{
			static std::uintptr_t hookedAddress = hkbClipGenerator::Activate.address() + 0x66E;

			struct Hook : Xbyak::CodeGenerator
			{
				Hook()
				{
					Xbyak::Label hookLabel;
					Xbyak::Label retnLabel;

					mov(rdx, r15);	// r15 = hkbContext*
					call(ptr[rip + hookLabel]);

					jmp(ptr[rip + retnLabel]);

					L(hookLabel), dq(reinterpret_cast<std::uintptr_t>(&hkbClipGenerator::ComputeStartTime_Hook));
					L(retnLabel), dq(hookedAddress + 5);
				}
			};

			hkbClipGenerator::ComputeStartTime = utils::WriteBranchTrampoline<5>(hookedAddress, Hook());
		}

		// hkbClipGenerator::Deactivate
		{
			static std::uintptr_t hookedAddress = hkbClipGenerator::Deactivate.address() + 0x1A;

			struct Hook : Xbyak::CodeGenerator
			{
				Hook(utils::ContextRegister context)
				{
					Xbyak::Label hookLabel;
					Xbyak::Label retnLabel;

					mov(rdx, context == utils::ContextRegister::rsi ? rsi : r14);
					call(ptr[rip + hookLabel]);

					jmp(ptr[rip + retnLabel]);

					L(hookLabel), dq(reinterpret_cast<std::uintptr_t>(&hkbClipGenerator::ResetIgnoreStartTime_Hook));
					L(retnLabel), dq(hookedAddress + 5);
				}
			};

			hkbClipGenerator::ResetIgnoreStartTime = utils::WriteBranchTrampoline<5>(hookedAddress, Hook(context));
		}

		// Character::ProcessMotionData
		{
			const auto translation1HookedAddress = motionAddress + motionOffsets[0];
			const auto translation2HookedAddress = motionAddress + motionOffsets[1];
			const auto rotation1HookedAddress = motionAddress + motionOffsets[2];
			const auto rotation2HookedAddress = motionAddress + motionOffsets[3];

			struct Hook : Xbyak::CodeGenerator
			{
				Hook(void* a_func)
				{
					Xbyak::Label hookLabel;

					sub(rsp, 0x28);

					mov(ptr[rsp + 0x20], r13);	// r13 = Character*
					mov(r9, rbx);				// SE: rbx = BSFixedString*
					if (REL::Module::IsAE())
					{
						sub(r9, 0x14);			// AE: rbx - 0x14 = BSFixedString*
					}
					call(ptr[rip + hookLabel]);

					add(rsp, 0x28);

					ret();

					L(hookLabel), dq(reinterpret_cast<std::uintptr_t>(a_func));
				}
			};

			// Translation
			utils::WriteCallTrampoline<5>(translation1HookedAddress, Hook(&MotionDataContainer::ProcessTranslationData_Hook));
			utils::WriteCallTrampoline<5>(translation2HookedAddress, Hook(&MotionDataContainer::ProcessTranslationData_Hook));

			//// Rotation
			utils::WriteCallTrampoline<5>(rotation1HookedAddress, Hook(&MotionDataContainer::ProcessRotationData_Hook));
			utils::WriteCallTrampoline<5>(rotation2HookedAddress, Hook(&MotionDataContainer::ProcessRotationData_Hook));
		}
		const auto configuredLevel = spdlog::default_logger()->level();
		logger::set_level(logger::level::info, logger::level::info);
		logger::info("Succesfully loaded!");
		logger::flush();
		logger::set_level(configuredLevel, configuredLevel);
		rollback.Commit();
		return true;
	}
}
