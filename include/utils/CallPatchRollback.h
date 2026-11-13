#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace utils
{
	class CallPatchRollback
	{
	public:
		using Sites = std::array<std::uintptr_t, 6>;
		using Restore = void (*)(std::uintptr_t, const void*, std::size_t);

		CallPatchRollback(Sites sites, Restore restore) : addresses(sites), write(restore)
		{
			if (!write) throw std::invalid_argument("Missing CALL restoration function");
			for (std::size_t i = 0; i < addresses.size(); ++i)
				std::memcpy(original[i].data(), reinterpret_cast<const void*>(addresses[i]), original[i].size());
		}

		CallPatchRollback(const CallPatchRollback&) = delete;
		CallPatchRollback& operator=(const CallPatchRollback&) = delete;

		~CallPatchRollback()
		{
			if (!committed) {
				for (std::size_t i = 0; i < addresses.size(); ++i)
					write(addresses[i], original[i].data(), original[i].size());
			}
		}

		void Commit() noexcept { committed = true; }

	private:
		Sites addresses;
		std::array<std::array<std::uint8_t, 5>, 6> original{};
		Restore write;
		bool committed{};
	};
}
