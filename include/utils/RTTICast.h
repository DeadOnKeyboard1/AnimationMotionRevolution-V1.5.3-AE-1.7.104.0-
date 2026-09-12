#pragma once

#include "RE/RTTI.h"

namespace amr
{
	namespace detail
	{
		template <class To, class From>
		struct cast_to_is_valid :
			std::conjunction<
				RE::detail::types_are_compat<To, From>,
				RE::detail::target_is_valid<To>,
				RE::detail::implements_rtti<To>>
		{};

		template <class To, class From>
		inline constexpr bool cast_to_is_valid_v = cast_to_is_valid<To, From>::value;
	}

	template <class T, std::enable_if_t<RE::detail::target_is_valid<T>::value, int> = 0>
	auto get_dynamic_cast_info(T* a_type)
	{
		auto vfTableAddr = *reinterpret_cast<std::uintptr_t*>(a_type);
		auto col = *reinterpret_cast<RE::RTTI::CompleteObjectLocator**>(vfTableAddr - sizeof(std::uintptr_t));
		return std::pair<decltype(col->typeDescriptor), std::uint32_t>{ col->typeDescriptor, col->offset };
	}

	template <class To, class From, std::enable_if_t<detail::cast_to_is_valid_v<To, From*>, int> = 0>
	To rtti_cast(From* a_from)
	{
		auto [from, vfDelta] = get_dynamic_cast_info(a_from);

		REL::Relocation<void*> to{ RE::detail::remove_cvpr_t<To>::RTTI };

		if (!from.get() || !to.get()) {
			return nullptr;
		}

		return static_cast<To>(
			RE::RTDynamicCast(
				const_cast<void*>(static_cast<const volatile void*>(a_from)),
				vfDelta,
				from.get(),
				to.get(),
				false));
	}
}
